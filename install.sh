#! /bin/sh
# Build SPECS from a fresh clone: Meson build -> tests.
# Needs a C++20 compiler, cmake, ninja, meson (>= 1.3), flex, bison (>= 3) and python3.
# `meson setup` downloads SystemC 3.0.2, taywee/args and GoogleTest (subprojects/*.wrap) and builds
# SystemC as C++20 (offline: run `meson subprojects download` beforehand). SPECS is built in
# build/ (override with BUILD_DIR). The result is build/specs.

set -e # Exit on error

cd "$(dirname "$0")"
BUILD_DIR=${BUILD_DIR:-build}

echo ------------------------------
echo Compiling SPECS
echo ------------------------------
if [ -d "$BUILD_DIR/meson-private" ]; then
    meson setup --reconfigure "$BUILD_DIR"
else
    meson setup "$BUILD_DIR"
fi
meson compile -C "$BUILD_DIR"

echo ------------------------------
echo Testing SPECS
echo ------------------------------
meson test -C "$BUILD_DIR" --print-errorlogs

echo ------------------------------
echo "All done! The simulator is $BUILD_DIR/specs"
echo ------------------------------
