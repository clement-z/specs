# PF02 progress (parameter sweeps and Monte-Carlo, F-02, branch `ws/sweeps`)

Heartbeat: `5e05ca80` (PF02-heartbeat, every 5 min, expires in 48 h). Delete it after the report is committed.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | Baseline, output format evaluation and design (this file) | done | 5c9f98f | baseline 92a4974: gcc 16/16, golden strict 256/256; before-recordings clang/asan-gcc/asan-clang in /tmp/pf02-before |
| 1 | Expression functions (`gauss`, `uniform`, `lognormal`, `choice`, math), per-name draw streams, `shared=` | done | 1ac820a | 6 gtest, 5 golden (new); strict 261/261 |
| 2 | `.step param` (linear, `list`, nested), `.mc N [seed=]`, run plan, `--run K`, `-n` plan listing | done | ef07d1e | items 2 and 3 in one commit (main.cpp couples them) |
| 3 | Runner: one process per run, run table (appendable columns + index), failures recorded, resume | done | ef07d1e | 7 gtest, 11 golden new; 3 help cases re-recorded (new options); strict 272/272; asan-clang pending |
| 4 | pyspecs reader | todo | | |
| 5 | Docs, CHANGELOG, golden cases, unit tests | todo | | |

## Output format: evaluation and choice (scope item 3, decided before implementing)

Need: one table for all runs (run index, stepped and drawn parameter values), many runs × probes × time
points; no heavy runtime dependency in `specs`; readable from Python without loading everything; resilient to
a failing run and resumable.

| Option | Large data | Append / resume | Lazy read in Python | Dependency |
|---|---|---|---|---|
| One `.npz` for all runs (existing writer) | yes | no: a zip with its central directory at the end, rewritten on every append; a crash loses the file | no: members are not memory-mappable through `np.load` | none |
| One `.npz` (or CSV) per run + an index | yes | yes (one file per run) | per run | none; but 1e5 runs = 1e5+ files, and it is not "one table" |
| CSV appended | 3-4x larger, slow to parse | yes | chunked `pandas.read_csv` only | none |
| HDF5 | yes | appendable datasets, but a crash during a write can corrupt the file (SWMR needed) | yes (h5py) | libhdf5 in `specs`, h5py in Python: heavy |
| Parquet / Arrow IPC | yes | Parquet: no append; Arrow IPC stream: yes | yes (pyarrow) | Arrow C++ or a hand-written flatbuffers writer: heavy |
| **Directory of appendable `.npy` columns + a run index (chosen)** | yes: binary, 8 bytes per float | yes: rows appended run by run; the index is the commit record, a crash is repaired by truncating to it | yes: `np.load(..., mmap_mode="r")`, rows of a run = a slice | none (NPY 1.0 is the format the `.npz` writer already produces) |

Choice: `<trace>.runs/`, a column store in NumPy's own format:
- `<column>.npy`: one 1-D array per column of the probe table (F-05 columns plus `run`): `run` (int32),
  `time_s`, `signal` (int32, index into `signal_names.txt`), `wavelength_m`, `power_W`, `field_re`, `field_im`.
  The NPY header is written with a fixed size (128 bytes), so the row count in it is rewritten in place after
  each append.
- `draw_run.npy`, `draw_name.npy` (int32, index into `draw_names.txt`), `draw_value.npy`: every random draw
  of every run (long format: one row per draw), e.g. the length of each ring instance of a CROW.
- `runs.csv`: the run index, one line per finished run, appended: run, status, exit code, seed, the stepped
  parameters, the drawn top-level `.param` values, row and draw ranges in the column files, message.
  Readers keep the last line of each run (a resumed run that failed before appends a new line).
- `table.manifest.json`: version, netlist files and hashes, command line, master seed, the plan (every run with
  its seed and stepped values); also the fingerprint checked when a run is resumed.
- `logs/run<k>.log`: stderr of a run, kept when not empty. Children's stdout is discarded (`--run K`
  reproduces it).

Commit order: a finished run is committed (rows appended, headers updated, then its `runs.csv` line) in run
order, whatever `-j` is, so the files do not depend on `-j`. A crash between two commits leaves extra rows past
the committed count: the next start truncates the columns to the count given by `runs.csv`.

pyspecs reads `runs.csv` with pandas and the columns with `mmap_mode="r"`: selecting runs (by index or by
parameter values) reads only their row ranges; a concatenated long DataFrame (run + parameters + probe columns)
is built only for what is selected.

## Design decisions (go into the report)
- Run isolation: **one process per run**, forked by `specs` after the netlist is parsed (no SystemC object
  exists yet, as in the gtest fork harness). The child re-parses the netlist with the run's parameter values
  and seed, simulates, writes a chunk file and exits; the parent appends the chunk to the table. A crash, an
  `exit(1)` or a signal in the child only fails that run.
- `.step param X start stop step`: values as `.dc` (`sweep_values()`: v += step, stop included when
  (stop-start)/step is an integer within 1e-9), `.step param X list v1 v2 ...`; several `.step` lines nest,
  the first one outermost; `.mc` is the innermost loop. A stepped parameter must exist as a top-level
  `.param`; a `--param` of the same name is an error.
- Random functions are evaluated when the line is parsed (like every expression). Stream of a draw:
  `rng_for(seed, "<instance path>/<line owner>/<assigned name>#<n>")`, owner = element name, `.param`,
  `.subckt <name>`, ...; n counts the draws of that base name in the same scope. Subcircuit bodies are parsed
  once per instance, so each instance draws independently; `shared=1` drops the instance path (one draw per
  subcircuit definition), `shared="name"` uses the stream `SHARED/<name>` (one value for every use of the name).
- Seeds: without `.mc`, every run uses the master seed (`--seed`, `.options seed`, default 0). With `.mc N`,
  run k of the Monte-Carlo loop uses seed s_k = first output of `rng_for(master, ".mc/<k>")` (also for PDET
  noise), the same for every step point (common random numbers). `.mc seed=S` sets the master seed like
  `.options seed`; `--seed` wins over both.
- `.options seed` is applied when its line is parsed (root netlist only), so that draws after it use it; a
  draw made before a later `.options seed` that changes the seed is an error (not silently wrong).
- `--run K`: runs plan point K alone, in-process, with the normal outputs (VCD, tables, manifest) and stdout:
  each run is reproducible on its own.
