# Flameshot Enhancements Documentation

This document summarizes the improvements made to the Flameshot color picker and magnifier components.

## 1. Advanced Color Picker (Side Panel)
- **RGBA Support**: Added dedicated sliders and spinboxes for Red, Green, Blue, and Alpha (transparency) channels.
- **Enhanced Hex Input**: Support for both 6-digit RGB and 8-digit ARGB hex codes.
- **RGBA CSS Display**: New field showing the color in `rgba(r, g, b, a)` format.
- **Auto-Copy Logic**: Clicking or selecting the Hex or RGBA fields instantly copies the value to the clipboard.
- **Transparency Preview**: The color preview label now features a procedural checkerboard background to visualize alpha levels.
- **Improved Layout**: Centered color wheel, organized RGB/Alpha group box, and optimized spacing.

## 2. Professional Magnifier (Grab Color)
- **Circular Design**: Replaced the square magnifier with a circular mask for a more authentic "magnifying glass" look.
- **Dynamic Zoom**: Support for mouse wheel zooming (2x to 50x) while picking colors.
- **Performance Optimizations**:
    - Cached screen image to eliminate movement lag.
    - Optimized paint events and window mask updates.
- **Pixel Grid & Focus**: Automatic grid at high zoom levels and a red focal square for pixel-perfect selection.
- **Increased Size**: Enlarged the magnifier window for better visibility.

## 3. Build Automation
- **AppImage Script**: Added `scripts/build_appimage.sh` to automate local AppImage creation.
- **Dependency Checks**: The script automatically verifies system requirements and provides installation commands for Linux Mint/Ubuntu.

## Files Modified
- `src/widgets/panel/sidepanelwidget.cpp` & `.h`
- `src/widgets/panel/colorgrabwidget.cpp` & `.h`
- `scripts/build_appimage.sh` (New)
- `.gitignore` (Updated)
