#!/bin/bash
# C-Express Installation Script
# This script builds and runs the C installer

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "C-Express Installation Script"
echo "=============================="
echo ""

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo "Creating build directory..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Always reconfigure to pick up any prefix changes
echo "Running CMake configuration..."
cmake ..

# Build the installer if needed
if [ ! -f "installer/cexpress-installer" ]; then
    echo "Building installer..."
    make cexpress-installer
fi

echo ""
echo "Running installer..."
echo ""

# Run the installer with all passed arguments
# Pass the current build directory to the installer
exec ./installer/cexpress-installer --build-dir "$BUILD_DIR" "$@"
