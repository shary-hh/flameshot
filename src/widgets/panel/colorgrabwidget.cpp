#include "colorgrabwidget.h"
#include "core/qguiappcurrentscreen.h"
#include "widgets/capture/overlaymessage.h"
#include "widgets/panel/sidepanelwidget.h"

#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QPainter>
#include <QScreen>
#include <QShortcut>
#include <QTimer>
#include <stdexcept>

// Width (= height) and zoom level of the widget before the user clicks
#define WIDTH1 120
#define ZOOM1 11
// Width (= height) and zoom level of the widget after the user clicks
#define WIDTH2 240
#define ZOOM2 15

// NOTE: WIDTH1(2) should be divisible by ZOOM1(2) for best precision.
//       WIDTH1 should be odd so the cursor can be centered on a pixel.

ColorGrabWidget::ColorGrabWidget(QPixmap* p, QWidget* parent)
  : QWidget(parent)
  , m_pixmap(p)
  , m_mousePressReceived(false)
  , m_extraZoomActive(false)
  , m_magnifierActive(false)
  , m_zoom(ZOOM1)
  , m_lastWidth(0)
{
    if (p == nullptr) {
        throw std::logic_error("Pixmap must not be null");
    }
    m_screenImage = m_pixmap->toImage();
    setAttribute(Qt::WA_DeleteOnClose);
    // We don't need this widget to receive mouse events because we use
    // eventFilter on other objects that do
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_QuitOnClose, false);
    // On Windows: don't activate the widget so CaptureWidget remains active
    setAttribute(Qt::WA_ShowWithoutActivating);
    setWindowFlags(Qt::BypassWindowManagerHint | Qt::WindowStaysOnTopHint |
                   Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    setMouseTracking(true);
}

void ColorGrabWidget::startGrabbing()
{
    // NOTE: grabMouse() would prevent move events being received
    // With this method we just need to make sure that mouse press and release
    // events get consumed before they reach their target widget.
    // This is undone in the destructor.
    qApp->setOverrideCursor(Qt::CrossCursor);
    qApp->installEventFilter(this);
    OverlayMessage::pushKeyMap(
      { { tr("Enter or Left Click"), tr("Accept color") },
        { tr("Hold Left Click"), tr("Precisely select color") },
        { tr("Space or Right Click"), tr("Toggle magnifier") },
        { tr("Esc"), tr("Cancel") } });
}

QColor ColorGrabWidget::color()
{
    return m_color;
}

bool ColorGrabWidget::eventFilter(QObject*, QEvent* event)
{
    // Consume shortcut events and handle key presses from whole app
    if (event->type() == QEvent::KeyPress ||
        event->type() == QEvent::Shortcut) {
        QKeySequence key = event->type() == QEvent::KeyPress
                             ? static_cast<QKeyEvent*>(event)->key()
                             : static_cast<QShortcutEvent*>(event)->key();
        if (key == Qt::Key_Escape) {
            emit grabAborted();
            finalize();
        } else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
            emit colorGrabbed(m_color);
            finalize();
        } else if (key == Qt::Key_Space && !m_extraZoomActive) {
            setMagnifierActive(!m_magnifierActive);
        }
        return true;
    } else if (event->type() == QEvent::MouseMove) {
        // NOTE: This relies on the fact that CaptureWidget tracks mouse moves

        if (m_extraZoomActive && !geometry().contains(cursorPos())) {
            setExtraZoomActive(false);
            return true;
        }
        if (!m_extraZoomActive && !m_magnifierActive) {
            // This fixes an issue when the mouse leaves the zoom area before
            // the widget even appears.
            hide();
        }
        if (!m_extraZoomActive) {
            // Update only before the user clicks the mouse, after the mouse
            // press the widget remains static.
            updateWidget();
        }

        // Hide overlay message when cursor is over it
        OverlayMessage* overlayMsg = OverlayMessage::instance();
        overlayMsg->setVisibility(
          !overlayMsg->geometry().contains(cursorPos()));

        m_color = getColorAtPoint(cursorPos());
        emit colorUpdated(m_color);
        return true;
    } else if (event->type() == QEvent::MouseButtonPress) {
        m_mousePressReceived = true;
        auto* e = static_cast<QMouseEvent*>(event);
        if (e->buttons() == Qt::RightButton) {
            setMagnifierActive(!m_magnifierActive);
        } else if (e->buttons() == Qt::LeftButton) {
            setExtraZoomActive(true);
        }
        return true;
    } else if (event->type() == QEvent::MouseButtonRelease) {
        if (!m_mousePressReceived) {
            // Do not consume event if it corresponds to the mouse press that
            // triggered the color grabbing in the first place. This prevents
            // focus issues in the capture widget when the color grabber is
            // closed.
            return false;
        }
        auto* e = static_cast<QMouseEvent*>(event);
        if (e->button() == Qt::LeftButton && m_extraZoomActive) {
            emit colorGrabbed(getColorAtPoint(cursorPos()));
            finalize();
        }
        return true;
    } else if (event->type() == QEvent::MouseButtonDblClick) {
        return true;
    } else if (event->type() == QEvent::Wheel) {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);
        if (wheelEvent->angleDelta().y() > 0) {
            m_zoom = qMin(50.0f, m_zoom + 1.0f);
        } else {
            m_zoom = qMax(2.0f, m_zoom - 1.0f);
        }
        updateWidget();
        return true;
    }
    return false;
}

void ColorGrabWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    if (!painter.isActive())
        return;

    int width = this->width();
    float zoom = m_zoom;
    QPoint adjustedCursorPos = cursorPos();

    QRect sourceRect(0, 0, width / zoom, width / zoom);
    sourceRect.moveCenter(adjustedCursorPos);

    painter.drawImage(rect(), m_screenImage, sourceRect);

    // Draw circular border
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::darkGray, 4));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(rect().adjusted(2, 2, -2, -2));

    // Draw pixel grid if zoom is high enough
    if (m_zoom >= 5.0f) {
        painter.setPen(QColor(128, 128, 128, 100));
        for (float x = 0; x <= width; x += m_zoom) {
            painter.drawLine(QPointF(x, 0), QPointF(x, height()));
        }
        for (float y = 0; y <= height(); y += m_zoom) {
            painter.drawLine(QPointF(0, y), QPointF(width, y));
        }
    }

    // Highlight the center pixel
    painter.setPen(QPen(Qt::red, 1));
    painter.setBrush(Qt::NoBrush);
    float centerX = (width - m_zoom) / 2.0f;
    float centerY = (height() - m_zoom) / 2.0f;
    painter.drawRect(QRectF(centerX, centerY, m_zoom, m_zoom));
}

void ColorGrabWidget::showEvent(QShowEvent*)
{
    updateWidget();
}

QPoint ColorGrabWidget::cursorPos() const
{
    return QCursor::pos(QGuiAppCurrentScreen().currentScreen());
}

/// @note The point is in screen coordinates.
QColor ColorGrabWidget::getColorAtPoint(const QPoint& p) const
{
    QPoint point = p;
#if defined(Q_OS_MACOS)
    QScreen* currentScreen = QGuiAppCurrentScreen().currentScreen();
    if (currentScreen) {
        point = QPoint((p.x() - currentScreen->geometry().x()) *
                         currentScreen->devicePixelRatio(),
                       (p.y() - currentScreen->geometry().y()) *
                         currentScreen->devicePixelRatio());
    }
#endif
    if (point.x() < 0 || point.y() < 0 || point.x() >= m_screenImage.width() || point.y() >= m_screenImage.height()) {
        return QColor();
    }
    return m_screenImage.pixelColor(point);
}

void ColorGrabWidget::setExtraZoomActive(bool active)
{
    m_extraZoomActive = active;
    if (active) {
        m_zoom = ZOOM2;
    } else {
        m_zoom = ZOOM1;
    }
    if (!active && !m_magnifierActive) {
        hide();
    } else {
        if (!isVisible()) {
            QTimer::singleShot(250, this, [this]() { show(); });
        } else {
            updateWidget();
        }
    }
}

void ColorGrabWidget::setMagnifierActive(bool active)
{
    m_magnifierActive = active;
    setVisible(active);
}

void ColorGrabWidget::updateWidget()
{
    int width = m_extraZoomActive ? WIDTH2 : WIDTH1;

    if (m_lastWidth != width) {
        setMask(QRegion(0, 0, width, width, QRegion::Ellipse));
        m_lastWidth = width;
        setFixedSize(width, width);
    }

    move(cursorPos() - QPoint(width / 2, width / 2));
    update();
}

void ColorGrabWidget::finalize()
{
    qApp->removeEventFilter(this);
    qApp->restoreOverrideCursor();
    OverlayMessage::pop();
    close();
}
