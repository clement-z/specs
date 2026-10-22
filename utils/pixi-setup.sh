#!/bin/sh
# Configure build-pixi/NAME with this pixi environment's compilers (pixi.toml task `setup`).
# Usage (inside the environment): pixi-setup.sh NAME [meson setup options...]
#   NAME: gcc, clang (release, -march=native unless -Dnative=false), asan-gcc, asan-clang (debug,
#   ASan+UBSan, -Dnative=false; `meson test` applies tests/sanitizers/lsan.supp).
# A build directory remembers its compilers: an existing build-pixi/NAME is accepted only if it was
# configured with this environment's compiler, otherwise the script stops and says to remove it.
# Options are only used when the directory is created (afterwards: meson configure build-pixi/NAME).
set -e

name=$1
[ $# -gt 0 ] && shift
case $name in
    gcc|asan-gcc) cc=gcc cxx=g++ ;;
    clang|asan-clang) cc=clang cxx=clang++ ;;
    *) echo "pixi-setup.sh: unknown build '$name' (gcc, clang, asan-gcc or asan-clang)" >&2; exit 2 ;;
esac
case $name in
    asan-*) set -- -Dbuildtype=debug -Db_sanitize=address,undefined -Db_lundef=false -Dnative=false "$@" ;;
esac

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
