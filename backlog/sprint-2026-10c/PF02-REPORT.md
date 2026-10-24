# PF02 report — parameter sweeps and Monte-Carlo (F-02), branch `ws/sweeps`

Base 92a4974 (`sprint/2026-10c`). Decision: "Decisions of 2026-10-23, third round", row "Sweeps and Monte-Carlo"
(`backlog/DECISIONS-2026-10-21.md`). Brief: `discover/scout/F-02-sweeps-and-monte-carlo.md` (status block added).
Design and format evaluation: `PF02-progress.md` (written before implementing, 5c9f98f).

## Items

| # | Item | Outcome | Commits | Tests | Golden impact |
|---|---|---|---|---|---|
| 1 | Functions in expressions: math (`sqrt exp log log10 sin cos tan asin acos atan abs floor ceil min max`), random `gauss`, `uniform`, `lognormal`, `choice`; per-name streams of the F-01 master seed; per-instance draws, `shared=1` / `shared="name"` | done | 1ac820a | gtest ExpressionFunctions (6) | 5 new cases (`regress/f02-functions(-seed)`, `err/f02-{options-seed-after-draw,subckt-default-draw,unknown-function}`) |
| 2 | `.step param X start stop step` (stop included as `.dc`), `.step param X list ...`, nested (first outermost); `.mc N [seed=S]` (innermost); `--run K`; `-n` lists the runs | done | ef07d1e | gtest RunPlan (4) | new cases below |
| 3 | One run table for all runs, `<output>.runs/`; one process per run (`-j N`); failures recorded, others continue; resume; `--overwrite` | done | ef07d1e, 982bee5, 1315d81 | gtest RunTable (3, incl. crash repair, interrupted creation) and a runner test with failing, crashing, chunk-less and resumed runs | 12 new cases (`regress/f02-step-{mc,mc-dry-run,mc-run,linear,failures}`, `err/f02-{step-unknown-param,step-bad-range,step-and-param,plan-format,run-out-of-range,run-without-plan,step-reserved-name}`); 3 existing cases re-recorded: `cli/help`, `cli/no-args`, `err/f15-no-coalescing-removed` (help text only: the new options `--run`, `-j/--jobs`, `--overwrite`) |
| 4 | pyspecs reader `pyspecs/runtable.py` (runs, select by run/parameter, `data()`, `iter_runs()`, `draws()`, memory-mapped columns) | done | 9aa4624, 81576b7 | `tests/tools/test_run_table.py` (Meson suite `tools`; skips with exit 77 without numpy/pandas) | — |
| 5 | Docs (`doc/netlist.md` "Functions", "`.step` and `.mc`", CLI; `doc/reproducibility.md`), CHANGELOG `[unreleased]`, README, golden README, golden harness `.npy` decoding (+ its unit tests), gtest case re-recorded (new tests only) | done | d6c680e, 24070fc, 1315d81, 4e2310b | `test_run_golden.py` (3 new) | gtest case: new tests only |

Validation from the brief, done by hand at ef07d1e and in tests: a table row equals `specs --run K --format npz`
and `--seed s_k --param n2=...` on the netlist without `.step`/`.mc`, bit for bit (npz arrays; VCDs identical but
for `$date`); `-j 1` and `-j 2` (reviewer: `-j 6`) give byte-identical tables; `.mc` with a fixed seed is
reproducible (golden `--repeat 2`).

## Output format (scope item 3)
Evaluated in `PF02-progress.md` (one `.npz`, per-run files + index, CSV, HDF5, Parquet/Arrow, appendable `.npy`
columns). Chosen: a directory of appendable NumPy `.npy` columns with fixed 128-byte headers (row count rewritten
in place), plus `runs.csv` (index and commit record), `signal_names.txt`, the draw table (`draw_*.npy`,
`draw_names.txt`), `table.manifest.json` and `logs/`. No new dependency in `specs` (same NPY 1.0 writer approach
as the `.npz`); `np.load(mmap_mode="r")` reads lazily; runs are committed in run order (files independent of
`-j`); a crash leaves extra rows that the next open cuts back to what `runs.csv` accounts for.

## Decisions I had to take (conservative; none changes existing results)
- **Run isolation: one process per run**, `fork()` after the netlist is parsed and before any SystemC
  elaboration; the child re-parses with the run's values and seed. stdout of a run is discarded (`--run K`
  reproduces it), stderr kept in `logs/run<k>.log` when not empty.
- **Seeds:** without `.mc` every run uses the master seed; Monte-Carlo run k uses `rng_for(master, ".mc/k").next()`
  for every draw (random functions and PDET noise), the same at every step point (common random numbers).
  `.mc seed=S` acts as `.options seed` (`--seed` wins); `.mc seed` and `.options seed` with different values
  is an error.
- **Stream names of draws:** `<instance path><owner>/<assigned name>#<n>` (owner = element, `.param`, directive);
  `shared=1` → `SUBCKT:<name>/...` (one draw per subcircuit definition), `shared="tag"` → `SHARED/tag`.
- **`.options seed` is applied when its line is parsed** (root netlist only) so that later draws use it; a seed
  changed after a draw is an error (rather than a silently unused seed). Invalid values are still reported
  by `OPTIONSDirective::create()` as before (strict golden unchanged).
- **Random functions in `.subckt` parameter defaults are an error** (the defaults are evaluated once for every
  instance, which would contradict "each instance draws independently").
- **Status:** `ok` / `failed` (non-zero exit, or exit 0 without a result) / `crashed` (signal). A `.tran`
  stopped at `maxevents` exits 0 by existing design: recorded `ok`, with the warning as message. `specs` exits
  1 at the end if any run failed (the table is complete).
- **Resume:** rerunning the same command (same `table.manifest.json`, `-j` and `--overwrite` excluded from its
  command line) skips runs recorded `ok` and reruns the others (new line appended; the last line counts).
  Another manifest is an error unless `--overwrite`; a directory that is not a run table is never touched.
- A stepped name must be a top-level `.param`; `--param` of the same name is an error; names that are
  `runs.csv` columns are an error. The parent validates the nominal netlist (parse, flatten, `.options`) once.
- Single-run output options (`--format`, `--no-trace`, `--json`, `--stats*`, `--progress`, `--print-manifest`)
  are errors with a plan unless `--run K`.
- Items 2 and 3 landed in one commit (ef07d1e): `main.cpp` couples the plan and the runner.
- `pyspecs` runners (`pyspecs.sweep()/mc()`, F-07) not done: the CLI runner plus the reader cover the scope.

## Review
Independent Opus review of the whole diff (read-only): no bug found in the fork runner, crash repair, run order or
determinism (it re-checked `-j` independence, crash repair, ASan on plans and parse errors, bison with no
conflicts). Its two real bugs were in the reader (pandas 3 `StringDtype`; `read_csv` 1-ulp error on 17-digit
values) → 81576b7. Risks/nits fixed in 1315d81: parse-error messages, children killed on parent errors,
interrupted creation, reserved names, `.options` checked once, reassigned drawn params, int64 overflow. Not
changed: no `fsync` (documented: a resume repairs process crashes, not power loss); overridden `.param`s still
evaluate their draw (recorded in the draw table, harmless); its "functions inside braces" claim did not reproduce
(`{sqrt(4)*1u}` parses).

## Found, not done
- **`.op` writes no table rows** (F-05: tables hold time samples; the operating point is the stdout listing), so a
  plan over an `.op` records statuses and draws only. Documented; use `.tran`/`.dc` or `--run K`.
- PDET and power-meter readouts are not in the tables (F-05); with a plan, specs now warns once.
- `cwsrc ... power=-1` aborts (`std::polar` assertion with `_GLIBCXX_ASSERTIONS`, SIGABRT) instead of a clean
  error; a run doing this is recorded `crashed`. Needs a parameter check (backlog item to open).
- Top-level parameters are not visible in subcircuit bodies (existing, documented): a drawn top-level value must
  be passed as an instance parameter.
- In a free expression (`key=value`, `.param`) a braced expression `{...}` is a variable reference only (existing
  grammar); functions are used unbraced there.
- The golden README counts were stale before this phase (`cir/*` 9 vs 12, etc.); updated to the actual counts.

## Final results (at 4e2310b)
- `pixi run test gcc`: 16 OK, 1 skipped (`run-table`: no pandas in the default environment).
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: **273/273** (17 new cases, 3 help cases
  re-recorded, no other change).
- `pixi run test asan-clang`: 16 OK, 1 skipped, no sanitizer message (after 982bee5; a UBSan finding, memcpy on
  an empty vector, was fixed there).
- Before/after (92a4974 vs ef07d1e+, `compare --rtol 0 --atol 0 --print-slack 0`): clang, asan-gcc,
  asan-clang: 252 same, 16 new, 3 differ = the help cases only. Counters (`stats-compare`, gcc): 261 same; the
  10 differing are new F-02 cases.
- `pixi run -e py python -I tests/tools/test_run_table.py --specs build-pixi/gcc/specs --repo .`: OK.
