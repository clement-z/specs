#!/bin/bash
# F-A7-14: CMakeLists.txt filters sources with REGEX ".*/test/.*" on the ABSOLUTE path, so a checkout that lives
# under any directory named tb/, parser/ or test/ configures with zero sources ("No SOURCES given to target: spx").
# usage: a7-cmake-path-regex.sh <repo> [prefix_path_with_systemc_and_args]
set -e
REPO=${1:?repo path}; DEPS=${2:-$HOME/.local/opt/specs-deps}
W=$(mktemp -d /tmp/a7-regex.XXXXXX); mkdir -p "$W/test/specs"
cp -r "$REPO/CMakeLists.txt" "$REPO/cmake" "$REPO/src" "$W/test/specs/"
cmake -S "$W/test/specs" -B "$W/test/specs/b" -G Ninja -DCMAKE_PREFIX_PATH="$DEPS" -DBUILD_TB=OFF \
      -DCMAKE_BUILD_TYPE=Release 2>&1 | grep -A2 "CMake Error" || echo "unexpected: configure succeeded"
echo "scratch: $W (remove when done)"
