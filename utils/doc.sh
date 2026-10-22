#!/bin/sh
# Build the Doxygen documentation (meson run_target `doc`, was `make doc`).
# Usage: doc.sh DOXYGEN REPO_ROOT OUT_DIR   -> OUT_DIR/html
# doc/Doxyfile uses paths relative to the repo root, so doxygen runs from there and only
# OUTPUT_DIRECTORY is overridden.
set -e
doxygen=$1
repo=$2
out=$3

echo "Generating documentation in $out/html"
mkdir -p "$out/html"
cp -r "$repo/doc/resources" "$out/html/"
cd "$repo"
{ cat doc/Doxyfile; echo "OUTPUT_DIRECTORY = $out"; } | "$doxygen" -
