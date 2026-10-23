# Scalable Photonic Event-driven Circuit Simulator (SPECS)

For the moment, refer to [the online documentation](https://demo.zrounba.fr/specs/doc/index.html
) to see how to download and use SPECS, or try it from the browser on our [demonstration website](https://demo.zrounba.fr/specs)

The netlist syntax (numbers and SI suffixes, parameters and `--param`, `.include`, subcircuits, `.op`/`.dc`/`.tran`)
and the element arguments (e.g. COUPLER `k`/`kp`/`t`) are documented in [`doc/netlist.md`](doc/netlist.md).
Tolerances and time resolution (`abstol`, `reltol`, `timescale`, ...) and the iteration cap of `.op`/`.dc`
(`maxiter`) are documented in [`doc/numerics.md`](doc/numerics.md).
Seeds (`--seed`, `.options seed`), the order of outputs and the run manifest written next to each trace are
documented in [`doc/reproducibility.md`](doc/reproducibility.md).

## Building

Requirements: a C++20 compiler (GCC ≥ 11 or clang), [Meson](https://mesonbuild.com) ≥ 1.3, Ninja, CMake,
flex, bison ≥ 3 and Python 3.

```sh
git clone https://github.com/clement-z/specs && cd specs
./install.sh                       # builds and tests build/specs (same as the commands below)
```

or by hand:

```sh
meson setup build                  # release build by default; downloads SystemC, args and googletest
meson compile -C build             # builds SystemC 3.0.2 as C++20, then specs
meson test -C build                # unit tests, testbench smoke tests, golden-output regression
build/specs -f circuits/add_drop.cir -o traces/out               # needs a traces/ directory
build/specs -f circuits/add_drop.cir -o traces/out --format csv  # traces/out.csv instead of traces/out.vcd
```

SystemC 3.0.2 and [taywee/args](https://github.com/Taywee/args) 6.4.7 come from `subprojects/*.wrap`
(release tarballs checked by sha256), like googletest. To build without network access, run
`meson subprojects download` beforehand, then `meson setup build --wrap-mode=nodownload`.

To use an installed SystemC instead (it must be built as **C++20**: a C++17 build fails to link and is
ignored with a warning), ask for it explicitly:

```sh
meson setup build -Dforce_fallback_for= -Dcmake_prefix_path=/path/to/systemc/prefix  # else the wrap
meson setup build --wrap-mode=nofallback -Dcmake_prefix_path=/path/to/systemc/prefix # else an error
```

SystemC is then looked up with pkg-config, then as a CMake package (`-Dcmake_prefix_path`); args as a CMake
package (with `nofallback`, the unit tests also need an installed googletest). Useful options: `-Dnative=true` (`-march=native`: faster on the build machine, but the binaries only run on CPUs like
it and results differ from the golden baseline in the last bits),
`-Dold_tb=false` (no built-in testbenches), `-Dtests=disabled` (no googletest download), `-Dsignal_ids=true`
(debug ids of optical signals, printed as `[NNNN]` in signal listings). The shared library
is built with `meson compile -C build spx` (or `-Dspx=true`, which also installs it), and the Doxygen docs with `ninja -C build doc`.
`meson install` installs `specs` and, when SystemC is built from its wrap, `libsystemc.so` into `<prefix>/<libdir>`
(the installed `specs` finds it through its rpath).

### Development environment (pixi, optional)

[pixi](https://pixi.sh) (one command, per-user install) provides the compilers (gcc 15, clang 21), Meson, Ninja,
CMake, flex and bison from `pixi.lock`, so nothing needs to be installed system-wide. The plain route above is
unchanged and remains the reference.

```sh
pixi run test                      # configure, build and meson test in build-pixi/gcc (gcc 15)
pixi run test clang                # same with clang 21 in build-pixi/clang; also asan-gcc, asan-clang
pixi run build-native              # -march=native build in build-pixi/gcc-native (also: test clang-native)
pixi run golden                    # golden-output check only; setup and build tasks exist too
pixi shell                         # shell with the environment (CC/CXX set to its gcc)
pixi run -e py jupyter lab         # Python environment for pyspecs and the notebooks
pixi run -e docs docs              # Doxygen HTML in build-pixi/doc/html
```

The pixi tasks only use `build-pixi/<toolchain>`, never `build/`. Extra Meson options go after `--`
(`pixi run setup gcc -- -Dspx=true`). Binaries built there link the environment's libstdc++ through an rpath, so
they run only while `.pixi/` exists; use the plain route for portable binaries. Only Linux is tested.
The pixi tasks also build googletest from its wrap (`-Dforce_fallback_for=systemc,args,gtest`): the environment's
pkg-config also reads the host's `/usr/lib/pkgconfig`, and an installed gtest found there does not compile with the
environment's compilers. Pass the same option to a plain `meson setup` inside `pixi shell`.

If you want to contribute with the project, check the `todo.txt` file for some inspiration on points to work on. We are also open for new ideas, so feel free to open an issue and discuss.

## Reference

If you use SPECS in your work, please cite:

> C. Zrounba, R. Cardoso, M. Gomes de Queiroz, P. Jimenez, M. Abdalla, A. Bosio, S. Le Beux, F. Pavanello, I. O'Connor,
> "Introducing SPECS: Scalable Photonic Event-driven Circuit Simulator", *49th European Conference on Optical Communications (ECOC)*, Glasgow, UK, Oct. 2023.
> [hal-04302200](https://hal.science/hal-04302200) — a copy is available in [`doc/papers/zrounba2023-specs-ecoc.pdf`](doc/papers/zrounba2023-specs-ecoc.pdf).

The same reference, with the software version, is in [`CITATION.cff`](CITATION.cff) (GitHub's "Cite this repository").

---
Copyright ©️ Institut des Nanotechnologies de Lyon (INL)
