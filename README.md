# Scalable Photonic Event-driven Circuit Simulator (SPECS)

For the moment, refer to [the online documentation](https://demo.zrounba.fr/specs/doc/index.html
) to see how to download and use SPECS, or try it from the browser on our [demonstration website](https://demo.zrounba.fr/specs)

## Building

Requirements: a C++20 compiler (GCC ≥ 11 or clang), [Meson](https://mesonbuild.com) ≥ 1.3, Ninja, CMake,
flex, bison ≥ 3 and Python 3.

```sh
git clone --recursive https://github.com/clement-z/specs && cd specs
./install.sh                       # SystemC -> .deps/systemc, then builds and tests build/specs
```

or by hand, with a SystemC 3.0.2 built as **C++20** (a C++17 build fails to link):

```sh
meson setup build -Dcmake_prefix_path=/path/to/systemc/prefix   # release build by default
meson compile -C build
meson test -C build                # unit tests, testbench smoke tests, golden-output regression
build/specs -f circuits/add_drop.cir -o traces/out               # needs a traces/ directory
```

Without `-Dcmake_prefix_path` (and no `systemc` pkg-config package), Meson builds SystemC from the
`thirdparty/systemc` submodule. Useful options: `-Dnative=false` (portable binaries, no `-march=native`),
`-Dold_tb=false` (no built-in testbenches), `-Dtests=disabled` (no googletest download). The shared library
is built with `meson compile -C build spx` (or `-Dspx=true`, which also installs it), and the Doxygen docs with `ninja -C build doc`.

If you want to contribute with the project, check the `todo.txt` file for some inspiration on points to work on. We are also open for new ideas, so feel free to open an issue and discuss.

## Reference

If you use SPECS in your work, please cite:

> C. Zrounba, R. Cardoso, M. Gomes de Queiroz, P. Jimenez, M. Abdalla, A. Bosio, S. Le Beux, F. Pavanello, I. O'Connor,
> "Introducing SPECS: Scalable Photonic Event-driven Circuit Simulator", *49th European Conference on Optical Communications (ECOC)*, Glasgow, UK, Oct. 2023.
> [hal-04302200](https://hal.science/hal-04302200) — a copy is available in [`doc/papers/zrounba2023-specs-ecoc.pdf`](doc/papers/zrounba2023-specs-ecoc.pdf).

---
Copyright ©️ Institut des Nanotechnologies de Lyon (INL)
