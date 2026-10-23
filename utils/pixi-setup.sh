#!/bin/sh
# Configure build-pixi/NAME with this pixi environment's compilers (pixi.toml task `setup`).
# Usage (inside the environment): pixi-setup.sh NAME [meson setup options...]
#   NAME: gcc, clang (release, portable: no -march=native, bit-identical to the golden baseline),
#   gcc-native, clang-native (release, -march=native: faster on this machine, last bits differ, B-183),
#   asan-gcc, asan-clang (debug, ASan+UBSan; `meson test` applies tests/sanitizers/lsan.supp).
# A build directory remembers its compilers: an existing build-pixi/NAME is accepted only if it was
# configured with this environment's compiler, otherwise the script stops and says to remove it.
# Options are only used when the directory is created (afterwards: meson configure build-pixi/NAME).
set -e

name=$1
[ $# -gt 0 ] && shift
case $name in
    gcc|gcc-native|asan-gcc) cc=gcc cxx=g++ ;;
    clang|clang-native|asan-clang) cc=clang cxx=clang++ ;;
    *) echo "pixi-setup.sh: unknown build '$name' (gcc, clang, gcc-native, clang-native, asan-gcc or asan-clang)" >&2
       exit 2 ;;
esac
case $name in
    *-native) set -- -Dnative=true "$@" ;;
    asan-*) set -- -Dbuildtype=debug -Db_sanitize=address,undefined -Db_lundef=false "$@" ;;
esac
# gtest from its wrap too (the project default forces only systemc and args): the environment's
# pkg-config also reads the host's /usr/lib/pkgconfig, and a host gtest.pc found there gives no
# include path the environment's compilers search. First, so a -Dforce_fallback_for=... given
# after `--` still wins.
set -- -Dforce_fallback_for=systemc,args,gtest "$@"

if [ -z "$CONDA_PREFIX" ]; then
    echo "pixi-setup.sh: run it through pixi (pixi run setup $name)" >&2
    exit 2
fi
cxx_path=$CONDA_PREFIX/bin/$cxx
if [ ! -x "$cxx_path" ]; then
    echo "pixi-setup.sh: $cxx is not part of this pixi environment ($CONDA_PREFIX)" >&2
    exit 2
fi
dir=build-pixi/$name

if [ -f "$dir/meson-info/intro-compilers.json" ]; then
    # Configured before: same C++ compiler binary (ccache wrapper aside)?
    configured=$(python -I -c 'import json, sys
exe = json.load(open(sys.argv[1]))["host"]["cpp"]["exelist"]
print(" ".join(exe))' "$dir/meson-info/intro-compilers.json")
    case " $configured " in
        *" $cxx_path "*)
            echo "$dir: already configured with $configured"
            exit 0 ;;
    esac
    echo "pixi-setup.sh: $dir was configured with '$configured', not with this environment's $cxx_path." >&2
    echo "A build directory keeps its compilers: remove it (rm -rf $dir) and run the task again." >&2
    exit 1
fi

# CC too: the SystemC subproject compiles C and assembly with it. Naming ccache keeps it in use
# (Meson only adds it by itself when CC/CXX are unset).
CC="ccache $CONDA_PREFIX/bin/$cc" CXX="ccache $cxx_path" exec meson setup "$dir" "$@"
