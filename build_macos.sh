#!/usr/bin/env bash
set -e

echo "=========================================="
echo " Building Dimension Sweeper for macOS"
echo "=========================================="

# Check for Homebrew
if ! command -v brew &>/dev/null; then
    echo "[!] Homebrew not found. Please install Homebrew from https://brew.sh or install dependencies manually."
else
    echo "[*] Checking and installing dependencies via Homebrew..."
    brew install cmake raylib enet 2>/dev/null || true
fi

# Create build directory
BUILD_DIR="build-macos"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "[*] Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "[*] Compiling..."
cmake --build . --config Release -j$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)

echo ""
echo "=========================================="
echo " Build Succeeded!"
echo " Executable: $BUILD_DIR/minesweeper"
echo " To run: cd $BUILD_DIR && ./minesweeper"
echo "=========================================="
