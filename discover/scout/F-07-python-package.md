# F-07: Packaged Python API (`pip install pyspecs`) with bundled binary

value 5 | effort M | risk med (depends on the Meson migration and on bundling SystemC) | prerequisites: Meson
(ws-meson), F-05 | horizon 2

## Problem / user story
*As a researcher I want `pip install pyspecs`, then `pyspecs.run("add_drop.cir")` returning a DataFrame.*
Every non-PCM workflow in the group is Python/PyTorch (Photontorch, TorchONN, custom numpy); SAX, Simphony,
PhotonForge and gdsfactory are Python-native; INTERCONNECT has `lumapi`. Today `pyspecs/` is a subprocess
wrapper (`pyspecs/specs.py:81-124`) that expects the binary in the working directory
(`simulator_directory + "specs"`, `:231-232`), defaults to `traces/delete_me.vcd` which becomes `.vcd.vcd`
(`:19`), imports numpy/pandas/matplotlib without using them (`:1-3`), ships no `pyproject`, and parses VCD with
a 2022 library and two known bugs (`specsparse.py:104, 146, 265`). The notebook expects `../specs`.

## Prior art
meson-python (0.22) + cibuildwheel (4.3) for compiled projects; `pysystemc` proves a wheel can bundle
`libsystemc.so`; SystemC in conda-forge/vcpkg/Conan/Homebrew/Debian is C++17-built; Simphony/SAX API shapes;
Luceda `get_time_response`; Scientific Python packaging guide (SOURCES C §1-2, 7).

## Proposed design
1. **Package layout** `python/pyspecs/` with `pyproject.toml` (meson-python backend, so one Meson tree builds
   the binary and installs it into the wheel's scripts directory). Runtime deps: numpy, pandas, pywellen;
   optional xarray, matplotlib.
2. **API**
   - `pyspecs.run(netlist, params=None, seed=None, out="table", workdir=None, timeout=None) -> Result`
     with `.probes` (long DataFrame), `.detectors`, `.op` (table from the OP printout or JSON), `.manifest`,
     `.stdout`, `.returncode`; raises `SpecsError` with the parser message and file:line when available.
   - `pyspecs.sweep(netlist, grid: dict, n_jobs=None)` and `pyspecs.mc(netlist, n, seed, n_jobs)` returning
     a long DataFrame with parameter columns (uses the F-02 CLI runner or a `multiprocessing` `spawn` pool with
     `maxtasksperchild=1`).
   - `pyspecs.Netlist` builder: `c = Netlist(); c.param(lambda0=1.55e-6); c.add("WG", "wg1", ["in","out"],
     L=1e-3, neff=2.3); c.analysis("tran", 3e-9); c.text()`; validated against the device registry (F-26) once
     it exists, string templates before.
   - `pyspecs.load(path)` for VCD/CSV/npz (F-05), `pyspecs.export_sdict()` (F-13), plotting helpers
     (`plot_sweep`, `plot_tran`).
   - `pyspecs.specs_binary()` resolves, in order: `SPECS_BIN`, the bundled executable, `PATH`.
3. **Wheels.** cibuildwheel on GitHub releases: manylinux_2_28 x86_64 and aarch64, macOS arm64 and x86_64
   (per-arch, since QuickThreads refuses universal2); SystemC compiled **statically with `-fPIC`** as a Meson
   subproject (`cmake.subproject()` on `thirdparty/systemc` with `CMAKE_CXX_STANDARD=20`) so nothing needs
   auditwheel repair; sdist for other platforms. Trusted publishing to PyPI.
4. **Examples.** Four notebooks executed in CI (nbmake): add-drop sweep, CROW pulses, PUF Monte-Carlo
   (after F-02), OCTANE cell programming.

## Touch points
`pyspecs/*.py` (rewrite into `python/pyspecs/`), new `pyproject.toml`, `meson.build` (`py.install_sources`,
executable install; coordinate with `discover/meson/PLAN.md`), `.github/workflows/release.yml`,
`src/main.cpp` exit codes (parser errors exit 1/3/134/139 today; map to a small stable set for the wrapper),
`README.md`, `pyspecs/pyspecs_example.ipynb` (replace).

## Validation plan
- `pip install .` in a clean venv; `pyspecs.run("circuits/add_drop.cir").probes` has 30 wavelength points
  and equals the golden VCD values.
- Wheel smoke test inside the cibuildwheel container (`python -c "import pyspecs; pyspecs.run(...)"`).
- Notebooks execute in CI; `pyspecs.sweep` on 4 cores scales.
- Type-checked (mypy) and documented (F-20).

## Effort breakdown
Package + API 4 d · wheels/CI 4 d · notebooks 2 d · docs 1 d. Total about 11 days after Meson lands.

## Open questions for the maintainer
- PyPI name (`pyspecs` may be taken; alternatives `specs-photonics`, `specs-sim`) and version policy (0.2.0).
- Is the "SystemC built as C++20" requirement real? If a C++17 `libsystemc` links and passes the golden check,
  conda-forge's `libsystemc` becomes usable; static vendoring sidesteps the question for wheels.
- Minimum Python: 3.9 (the lab machine) now, 3.12 stable ABI when the in-process extension (F-16) arrives?
