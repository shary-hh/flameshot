#!/bin/bash

# Flameshot AppImage Build Script
# This script automates the creation of a Flameshot AppImage locally.

set -e

# Configuration
PRODUCT="flameshot"
WORKDIR=$(pwd)

# 1. Check for dependencies
MISSING_DEPS=()
for dep in cmake g++ make wget; do
    if ! command -v $dep &> /dev/null; then
        MISSING_DEPS+=($dep)
    fi
done

if [ ${#MISSING_DEPS[@]} -ne 0 ]; then
    echo "Error: The following dependencies are missing: ${MISSING_DEPS[*]}"
    echo "Please install them using:"
    echo "  sudo apt update && sudo apt install g++ cmake build-essential qt6-base-dev qt6-tools-dev-tools qt6-svg-dev qt6-tools-dev libqt6svg6-dev qt6-l10n-tools qt6-wayland libgl-dev"
    exit 1
fi

APPDIR="${WORKDIR}/Flameshot.AppDir"
BUILD_DIR="${WORKDIR}/build-appimage"
TOOLS_DIR="${WORKDIR}/tools-appimage"

echo "=== Starting Flameshot AppImage Build ==="

# 1. Prepare directories
mkdir -p "${TOOLS_DIR}"
mkdir -p "${BUILD_DIR}"
rm -rf "${APPDIR}"
mkdir -p "${APPDIR}"

# 2. Download tools if not present
cd "${TOOLS_DIR}"
if [ ! -f "linuxdeploy-x86_64.AppImage" ]; then
    echo "Downloading linuxdeploy..."
    wget -c -nv "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage"
    chmod a+x linuxdeploy-x86_64.AppImage
fi

if [ ! -f "linuxdeploy-plugin-qt-x86_64.AppImage" ]; then
    echo "Downloading linuxdeploy-plugin-qt..."
    wget -c -nv "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage"
    chmod a+x linuxdeploy-plugin-qt-x86_64.AppImage
fi
cd "${WORKDIR}"

# 3. Compile Flameshot
echo "Compiling Flameshot..."
cmake -S . -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DUSE_LAUNCHER_ABSOLUTE_PATH:BOOL=OFF

cmake --build "${BUILD_DIR}" -j$(nproc)

# 4. Install to temporary AppDir
echo "Installing to AppDir..."
DESTDIR="${APPDIR}" cmake --install "${BUILD_DIR}"

# 5. Cleanup unnecessary files
echo "Cleaning up AppDir..."
rm -rf "${APPDIR}/usr/include"
rm -rf "${APPDIR}/usr/lib/cmake"

# 6. Generate AppImage
echo "Generating final AppImage..."

# Export environment variables for linuxdeploy
export QMAKE=$(which qmake6 || which qmake || echo "/usr/lib/qt6/bin/qmake")
export LD_LIBRARY_PATH="${APPDIR}/usr/lib:${LD_LIBRARY_PATH}"

# Run linuxdeploy
./tools-appimage/linuxdeploy-x86_64.AppImage \
    --appdir "${APPDIR}" \
    --plugin qt \
    --output appimage

echo "=== Success! AppImage generated in: ${WORKDIR} ==="
ls -lh Flameshot-*.AppImage
