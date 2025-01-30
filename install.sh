#! /bin/sh

set -e # Exit on error

SRC_DIR=$(pwd)

echo ------------------------------
echo Dowloading required submodules
echo ------------------------------
git submodule update --init --recursive --single-branch

echo ------------------------------
echo Compiling SystemC
echo ------------------------------
cd "$SRC_DIR/thirdparty/systemc"
mkdir -p build
cd build
cmake .. -DCMAKE_CXX_STANDARD=20 -DCMAKE_INSTALL_PREFIX=install
make -j install

echo ------------------------------
echo Compiling SPECS
echo ------------------------------
cd ../../..
specs_dir=$(pwd)
mkdir -p build
cd build
cmake .. -DBUILD_TB=1
make -j

echo ------------------------------
echo All done!
echo ------------------------------
