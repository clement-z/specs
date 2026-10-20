#!/bin/sh
# Build and install SystemC from the thirdparty/systemc submodule, as C++20: SystemC encodes the
# C++ standard in a link-time symbol, so it must match SPECS (cpp_std=gnu++20).
# Does nothing if PREFIX already holds the same SystemC commit built by the same compiler
# (stamp file), e.g. when restored from a CI cache. Used by install.sh and CI.
# The build tree is not registered in the CMake user package registry (~/.cmake/packages).
#
# Usage: utils/build-systemc.sh PREFIX [BUILD_DIR]     (compiler: $CXX, default c++)
set -e
prefix=$1
build=${2:-.deps/systemc-build}
if [ -z "$prefix" ]; then
    echo "usage: $0 PREFIX [BUILD_DIR]" >&2
    exit 2
fi
src=$(cd "$(dirname "$0")/.." && pwd)/thirdparty/systemc
if [ ! -f "$src/CMakeLists.txt" ]; then
    echo "thirdparty/systemc is empty: run 'git submodule update --init --recursive'" >&2
    exit 1
fi

# Compiler identity without the program name ("c++ (GCC) 13.2" and "g++ (GCC) 13.2" are the same)
stamp="$(git -C "$src" rev-parse HEAD) $(${CXX:-c++} --version | head -n 1 | sed 's/^[^ ]* //')"
if [ -f "$prefix/.specs-systemc-stamp" ] && [ "$(cat "$prefix/.specs-systemc-stamp")" = "$stamp" ]; then
    echo "SystemC already installed in $prefix ($stamp)"
    exit 0
fi

cmake -S "$src" -B "$build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20 -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DCMAKE_INSTALL_PREFIX="$prefix" \
    -DENABLE_EXAMPLES=OFF -DDISABLE_COPYRIGHT_MESSAGE=ON \
    -DCMAKE_EXPORT_NO_PACKAGE_REGISTRY=ON
cmake --build "$build"
cmake --install "$build"
printf '%s\n' "$stamp" > "$prefix/.specs-systemc-stamp"
