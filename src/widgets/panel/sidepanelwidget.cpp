// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "sidepanelwidget.h"
#include "utils/colorutils.h"
#include "utils/pathinfo.h"
#include "widgets/panel/colorgrabwidget.h"
#include "widgets/panel/utilitypanel.h"

#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShortcut>
#include <QSlider>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QGroupBox>
#if defined(Q_OS_MACOS)
#include <QScreen>
#endif

SidePanelWidget::SidePanelWidget(QPixmap* p, QWidget* parent)
  : QWidget(parent)
  , m_layout(new QVBoxLayout(this))
  , m_pixmap(p)
{

    if (parent != nullptr) {
        parent->installEventFilter(this);
    }

    auto* colorLayout = new QGridLayout();

    // Create Active Tool Size
    auto* toolSizeHBox = new QHBoxLayout();
    auto* activeToolSizeText = new QLabel(tr("Active tool size: "));

    m_toolSizeSpin = new QSpinBox(this);
    m_toolSizeSpin->setRange(1, maxToolSize);
    m_toolSizeSpin->setSingleStep(1);
    m_toolSizeSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    toolSizeHBox->addWidget(activeToolSizeText);
    toolSizeHBox->addWidget(m_toolSizeSpin);

    m_toolSizeSlider = new QSlider(Qt::Horizontal);
    m_toolSizeSlider->setRange(1, maxToolSize);
    m_toolSizeSlider->setValue(m_toolSize);
    m_toolSizeSlider->setMinimumWidth(minSliderWidth);

    colorLayout->addLayout(toolSizeHBox, 0, 0);
    colorLayout->addWidget(m_toolSizeSlider, 1, 0);

    // Create Active Color
    auto* colorHBox = new QHBoxLayout();
    auto* colorText = new QLabel(tr("Active Color: "));

    m_colorLabel = new QLabel();
    m_colorLabel->setFixedSize(100, 30);
    m_colorLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    m_colorLabel->setLineWidth(1);

    colorHBox->addWidget(colorText);
    colorHBox->addWidget(m_colorLabel);
    colorHBox->addStretch();
    colorLayout->addLayout(colorHBox, 2, 0);

    m_layout->addLayout(colorLayout);

    QColor background = this->palette().window().color();
    bool isDark = ColorUtils::colorIsDark(background);
    QString modifier =
      isDark ? PathInfo::whiteIconPath() : PathInfo::blackIconPath();
    QIcon grabIcon(modifier + "colorize.svg");
    m_colorGrabButton = new QPushButton(grabIcon, tr("Grab Color"));

    m_layout->addWidget(m_colorGrabButton);
    m_colorWheel = new color_widgets::ColorWheel(this);
    m_colorWheel->setColor(m_color);
    m_colorWheel->setMinimumSize(180, 180);
    m_layout->addWidget(m_colorWheel, 0, Qt::AlignCenter);

    // Create RGB and Alpha controls
    auto* rgbLayout = new QGridLayout();

    m_redSpin = new QSpinBox(this);
    m_greenSpin = new QSpinBox(this);
    m_blueSpin = new QSpinBox(this);
    m_alphaSpin = new QSpinBox(this);

    m_redSlider = new QSlider(Qt::Horizontal, this);
    m_greenSlider = new QSlider(Qt::Horizontal, this);
    m_blueSlider = new QSlider(Qt::Horizontal, this);
    m_alphaSlider = new QSlider(Qt::Horizontal, this);

    QString labelStyle = "QLabel { font-weight: bold; }";
    auto addRgbRow = [&](const QString& name, QSlider* slider, QSpinBox* spin, int row) {
        auto* label = new QLabel(name);
        label->setStyleSheet(labelStyle);
        slider->setRange(0, 255);
        spin->setRange(0, 255);
        rgbLayout->addWidget(label, row, 0);
        rgbLayout->addWidget(slider, row, 1);
        rgbLayout->addWidget(spin, row, 2);
    };

    addRgbRow("R", m_redSlider, m_redSpin, 0);
    addRgbRow("G", m_greenSlider, m_greenSpin, 1);
    addRgbRow("B", m_blueSlider, m_blueSpin, 2);
    addRgbRow("A", m_alphaSlider, m_alphaSpin, 3);

    auto* rgbGroup = new QGroupBox(tr("RGB / Alpha"), this);
    rgbGroup->setLayout(rgbLayout);
    m_layout->addWidget(rgbGroup);

    m_colorHex = new QLineEdit(this);
    m_colorHex->setAlignment(Qt::AlignCenter);
    m_colorHex->setToolTip(tr("Hex Color (Double click to copy)"));
    m_layout->addWidget(m_colorHex);

    m_colorRgba = new QLineEdit(this);
    m_colorRgba->setAlignment(Qt::AlignCenter);
    m_colorRgba->setReadOnly(true);
    m_colorRgba->setToolTip(tr("RGBA Color (Double click to copy)"));
    m_layout->addWidget(m_colorRgba);

    // Copy on double click
    connect(m_colorHex, &QLineEdit::selectionChanged, this, [=, this]() {
        if (m_colorHex->hasSelectedText()) {
             QApplication::clipboard()->setText(m_colorHex->text());
        }
    });

    connect(m_colorRgba, &QLineEdit::selectionChanged, this, [=, this]() {
        if (m_colorRgba->hasSelectedText()) {
             QApplication::clipboard()->setText(m_colorRgba->text());
        }
    });

    QHBoxLayout* gridHBoxLayout = new QHBoxLayout();
    m_gridCheck = new QCheckBox(tr("Display grid"), this);
    m_gridSizeSpin = new QSpinBox(this);
    m_gridSizeSpin->setRange(5, 50);
    m_gridSizeSpin->setSingleStep(5);
    m_gridSizeSpin->setValue(10);
    m_gridSizeSpin->setDisabled(true);
    m_gridSizeSpin->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    gridHBoxLayout->addWidget(m_gridCheck);
    gridHBoxLayout->addWidget(m_gridSizeSpin);
    m_layout->addLayout(gridHBoxLayout);

    // tool size sigslots
    connect(m_toolSizeSpin,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this,
            &SidePanelWidget::toolSizeChanged);
    connect(m_toolSizeSlider,
            &QSlider::valueChanged,
            this,
            &SidePanelWidget::toolSizeChanged);
    connect(this,
            &SidePanelWidget::toolSizeChanged,
            this,
            &SidePanelWidget::onToolSizeChanged);
    // color hex editor sigslots
    connect(m_colorHex, &QLineEdit::editingFinished, this, [=, this]() {
#if QT_VERSION < QT_VERSION_CHECK(6, 4, 0)
        if (!QColor::isValidColor(m_colorHex->text())) {
#else
        if (!QColor::isValidColorName(m_colorHex->text())) {
#endif
            m_colorHex->setText(m_color.name(QColor::HexRgb));
        } else {
            emit colorChanged(m_colorHex->text());
        }
    });
    // color grab button sigslots
    connect(m_colorGrabButton,
            &QPushButton::pressed,
            this,
            &SidePanelWidget::startColorGrab);
    // color wheel sigslots
    //   re-emit ColorWheel::colorSelected as SidePanelWidget::colorChanged
    connect(m_colorWheel,
            &color_widgets::ColorWheel::colorSelected,
            this,
            &SidePanelWidget::colorChanged);
    // Grid feature
    connect(m_gridCheck, &QCheckBox::clicked, this, [=, this](bool b) {
        this->m_gridSizeSpin->setEnabled(b);
        emit this->displayGridChanged(b);
    });
    connect(m_gridSizeSpin,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            &SidePanelWidget::gridSizeChanged);

    // RGB and Alpha sigslots
    auto connectRgb = [&](QSlider* slider, QSpinBox* spin) {
        connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), slider, &QSlider::setValue);
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, &SidePanelWidget::onRgbChanged);
    };

    connectRgb(m_redSlider, m_redSpin);
    connectRgb(m_greenSlider, m_greenSpin);
    connectRgb(m_blueSlider, m_blueSpin);
    connectRgb(m_alphaSlider, m_alphaSpin);

    m_layout->addStretch();
}

void SidePanelWidget::onColorChanged(const QColor& color)
{
    m_color = color;
    updateColorNoWheel(color);
    m_colorWheel->setColor(color);
}

void SidePanelWidget::onToolSizeChanged(int t)
{
    m_toolSize = qBound(0, t, maxToolSize);
    m_toolSizeSlider->setValue(m_toolSize);
    m_toolSizeSpin->setValue(m_toolSize);
}

void SidePanelWidget::startColorGrab()
{
    m_revertColor = m_color;
    m_colorGrabber = new ColorGrabWidget(m_pixmap);
    connect(m_colorGrabber,
            &ColorGrabWidget::colorUpdated,
            this,
            &SidePanelWidget::onTemporaryColorUpdated);
    connect(m_colorGrabber,
            &ColorGrabWidget::colorGrabbed,
            this,
            &SidePanelWidget::onColorGrabFinished);
    connect(m_colorGrabber,
            &ColorGrabWidget::grabAborted,
            this,
            &SidePanelWidget::onColorGrabAborted);

    emit hidePanel();
    m_colorGrabber->startGrabbing();
}

void SidePanelWidget::onColorGrabFinished()
{
    finalizeGrab();
    m_color = m_colorGrabber->color();
    emit colorChanged(m_color);
}

void SidePanelWidget::onColorGrabAborted()
{
    finalizeGrab();
    // Restore color that was selected before we started grabbing
    onColorChanged(m_revertColor);
}

void SidePanelWidget::onTemporaryColorUpdated(const QColor& color)
{
    updateColorNoWheel(color);
}

void SidePanelWidget::finalizeGrab()
{
    emit showPanel();
}

void SidePanelWidget::updateColorNoWheel(const QColor& c)
{
    // Create checkerboard background for transparency preview
    QPixmap checkerboard(16, 16);
    QPainter p(&checkerboard);
    p.fillRect(0, 0, 8, 8, Qt::lightGray);
    p.fillRect(8, 8, 8, 8, Qt::lightGray);
    p.fillRect(8, 0, 8, 8, Qt::white);
    p.fillRect(0, 8, 8, 8, Qt::white);
    p.end();

    QSize labelSize = m_colorLabel->size();
    if (labelSize.width() <= 0 || labelSize.height() <= 0) {
        labelSize = QSize(100, 30);
    }
    QPixmap preview(labelSize);
    preview.fill(Qt::transparent);
    QPainter previewPainter(&preview);
    previewPainter.drawTiledPixmap(preview.rect(), checkerboard);
    previewPainter.fillRect(preview.rect(), c);
    previewPainter.end();

    m_colorLabel->setPixmap(preview);

    // Update Hex input (support #RRGGBBAA if alpha < 255)
    if (c.alpha() < 255) {
        m_colorHex->setText(c.name(QColor::HexArgb));
    } else {
        m_colorHex->setText(c.name(QColor::HexRgb));
    }

    m_colorRgba->setText(QString("rgba(%1, %2, %3, %4)")
                         .arg(c.red())
                         .arg(c.green())
                         .arg(c.blue())
                         .arg(QString::number(c.alpha() / 255.0, 'f', 2)));

    // Update RGB spinboxes and sliders without triggering signals
    m_redSpin->blockSignals(true);
    m_greenSpin->blockSignals(true);
    m_blueSpin->blockSignals(true);
    m_alphaSpin->blockSignals(true);
    m_redSlider->blockSignals(true);
    m_greenSlider->blockSignals(true);
    m_blueSlider->blockSignals(true);
    m_alphaSlider->blockSignals(true);

    m_redSpin->setValue(c.red());
    m_greenSpin->setValue(c.green());
    m_blueSpin->setValue(c.blue());
    m_alphaSpin->setValue(c.alpha());
    m_redSlider->setValue(c.red());
    m_greenSlider->setValue(c.green());
    m_blueSlider->setValue(c.blue());
    m_alphaSlider->setValue(c.alpha());

    m_redSpin->blockSignals(false);
    m_greenSpin->blockSignals(false);
    m_blueSpin->blockSignals(false);
    m_alphaSpin->blockSignals(false);
    m_redSlider->blockSignals(false);
    m_greenSlider->blockSignals(false);
    m_blueSlider->blockSignals(false);
    m_alphaSlider->blockSignals(false);
}

void SidePanelWidget::onRgbChanged()
{
    QColor c(m_redSpin->value(),
             m_greenSpin->value(),
             m_blueSpin->value(),
             m_alphaSpin->value());
    if (c != m_color) {
        emit colorChanged(c);
    }
}

void SidePanelWidget::onAlphaChanged()
{
    onRgbChanged();
}

bool SidePanelWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        // Override Escape shortcut from CaptureWidget
        auto* e = static_cast<QKeyEvent*>(event);
        if (e->key() == Qt::Key_Escape && m_colorHex->hasFocus()) {
            m_colorHex->clearFocus();
            e->accept();
            return true;
        }
    } else if (event->type() == QEvent::MouseButtonPress) {
        // Clicks outside of the Color Hex editor
        m_colorHex->clearFocus();
    }
    return QWidget::eventFilter(obj, event);
}

void SidePanelWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    m_colorHex->clearFocus();
}
