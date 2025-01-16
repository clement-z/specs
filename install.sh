#! /bin/sh

set -e 

# Download required submodules
echo ------------------------------
echo Dowloading required submodules
echo ------------------------------
git submodule update --init --recursive --single-branch

# Compiling the systemc submodule

echo ------------------------------
echo Changing into SystemC folder
echo ------------------------------
cd ./thirdparty/systemc

if [ -d "./objdir" ] 
then
    echo "objdir already exists, not recreating." 
else
    mkdir objdir
fi


cd objdir

echo ------------------------------
echo Configuring SystemC
echo ------------------------------
cmake .. -DCMAKE_CXX_STANDARD=17 -DCMAKE_INSTALL_PREFIX=../sc_install -DCMAKE_INSTALL_DOCDIR=../sc_doc
make -j2
make install



echo ------------------------------
echo Compiling SPECS
echo ------------------------------
cd ../../..
specs_dir=$(pwd)
make -j2

echo ------------------------------
echo Finished!
echo ------------------------------