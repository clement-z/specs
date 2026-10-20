# B-088: Notebook calls `../specs`, which no build produces any more (CMake and the planned Meson build put it in `build/`)
severity: low | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 0e8db9c
location: pyspecs/pyspecs_example.ipynb (cell 0 source, cells 2-3 calls)
## Problem
Cell 0 uses one variable for two things: `path_to_specs = "../"` is both the `sys.path` root for `import pyspecs` and the `simulator_directory`. The binary is `build/specs` since the CMake switch (the `./specs` of the old Makefile layout is git-ignored and gone); the committed output of cell 2 still shows `Command: ../specs -f ../circuits/add_drop_pyspecs.cir ...`. Cells 2-3 also pin `abstol=1e-8, reltol=1e-4` (see B-079). The notebook output file `../traces/delete_me` overwrites the tracked placeholder `traces/delete_me.vcd` (B-007). Kernel metadata says Python 3.8.10 and the stored outputs include a PNG.
## Evidence
```
cell 0:  path_to_specs = "../" ; sys.path.insert(0, os.path.realpath(path_to_specs))
cell 2:  specs.simulate(netlist_file=..., simulator_directory=path_to_specs, abstol=1e-8, reltol=1e-4, output_file="../traces/delete_me")
CMakeLists.txt:49 add_executable(${PROJECT_NAME} ...) with no RUNTIME_OUTPUT_DIRECTORY; install.sh builds in build/; PLAN.md:107 keeps build/specs
```
## Expected
The notebook runs from `pyspecs/` after a standard build.
## Suggested fix
In cell 0 keep `path_to_specs = "../"` for `sys.path` and add `simulator_directory = os.path.join(path_to_specs, "build") + os.sep`; use it in cells 2 and 3 instead of `path_to_specs`. Edit the JSON source lines only; do not re-execute (outputs would change). Needs B-086 if `os.sep` is dropped.
## Golden impact
none.
## Related
B-079, B-086, B-007.

- Discovery seeds: S3.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-13.
## Verification
Read the notebook cells with `json.load`; file layout and CMake output location checked. Not re-run end to end (jupyter not installed), but the same calls were executed from Python with `simulator_directory="/tmp/ws-review-build/"`.

**Independent second check (V5, of F-A6-13): CONFIRMED.** Method: extracted and exec'd all notebook code cells from pyspecs/ in a scratch tree before and after the JSON edit (jupyter is not installed)

Before: cell 2 raises FileNotFoundError '../specs' (no build produces it). After the edit (new line `simulator_directory = os.path.join(path_to_specs, "build") + os.sep` after the `sys.path.insert` line, and `simulator_directory=simulator_directory` in cells 2 and 3) with build/specs present: cells 0-9 all run, Command is `../build/specs ...`, rc 0, "Type of simulation: TD". The notebook JSON round-trips byte for byte with `json.dumps(nb, indent=1, ensure_ascii=False) + "\n"`, so a fixer may edit it via json.load/dump this way. Without `ensure_ascii=False` or the trailing newline the whole file diffs. Stored cell outputs still show `../specs`; that is expected (do not re-execute). Note: cell 0 also contains the `# Dependencies:` comment that B-089 mentions, so combine the edits if both touch it.
