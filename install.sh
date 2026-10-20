#! /bin/sh
# Build SPECS from a fresh clone: submodules -> SystemC (C++20) -> Meson build -> tests.
# Needs a C++20 compiler, cmake, ninja, meson (>= 1.3), flex, bison (>= 3) and python3.
# SystemC is installed into .deps/systemc (override with SYSTEMC_PREFIX); SPECS is built in
# build/ (override with BUILD_DIR). The result is build/specs.

set -e # Exit on error

cd "$(dirname "$0")"
SYSTEMC_PREFIX=${SYSTEMC_PREFIX:-$(pwd)/.deps/systemc}
BUILD_DIR=${BUILD_DIR:-build}

echo ------------------------------
echo Downloading required submodules
echo ------------------------------
git submodule update --init --recursive --single-branch

echo ------------------------------
echo Compiling SystemC
echo ------------------------------
utils/build-systemc.sh "$SYSTEMC_PREFIX" .deps/systemc-build

echo ------------------------------
echo Compiling SPECS
echo ------------------------------
if [ -d "$BUILD_DIR/meson-private" ]; then
    meson setup --reconfigure "$BUILD_DIR" -Dcmake_prefix_path="$SYSTEMC_PREFIX"
else
    meson setup "$BUILD_DIR" -Dcmake_prefix_path="$SYSTEMC_PREFIX"
fi
meson compile -C "$BUILD_DIR"

echo ------------------------------
echo Testing SPECS
echo ------------------------------
meson test -C "$BUILD_DIR" --print-errorlogs

echo ------------------------------
echo "All done! The simulator is $BUILD_DIR/specs"
echo ------------------------------
