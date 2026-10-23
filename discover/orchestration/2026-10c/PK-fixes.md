# Phase K — fixes before v0.4.0 (branch `ws/kfixes`)

Follow `COMMON.md`. Phase id `PK`. Decisions: `backlog/DECISIONS-2026-10-21.md`, row "Before v0.4.0" (2026-10-23).
One commit per item (plus its golden re-records), in this order. Write your predictions (golden cases, counters)
in `PK-progress.md` before each re-record.

## Scope
1. **B-189** (`backlog/B-189-*.md`): a CWSRC power that is negative or not finite is a clean error at element
   creation (`Error: ...`, exit 1), like the other parameter checks; one `err/` golden case. Read B-063 (VLSRC
   negative power): include it only if its fix is the same kind of check with no open design question, else
   leave it and say why.
2. **B-193** (`backlog/B-193-*.md`): the run-table test (`tests/tools/test_run_table.py`, needs numpy and pandas)
   must run in CI and in pixi builds, and keep skipping (exit 77) where they are missing. Choose the smallest
   robust way (e.g. numpy and pandas installed in the CI jobs that run `meson test`, and available to the
   Python that the pixi builds' `meson test` uses, or that test run with the `py` environment's Python when it
   exists) and say why. If `pixi.lock` must change, update it with pixi and run `pixi install --locked` after.
3. **B-191** (`backlog/B-191-*.md`): netlist lines longer than flex's buffer (about 16 KB) must parse. Prefer
   removing what forces `REJECT` in `src/parser/parser.l` if it can be done with the golden outputs bit-identical
   (the scanner's behaviour must not change otherwise); if not, enlarge the buffer and give a clear error naming
   the limit and suggesting a value file. Golden case with a long inline VLSRC list (e.g. 2000 entries, ~47 KB).
4. **B-192** (`backlog/B-192-*.md`): VLSRC and EVLSRC value files are resolved like `.include` and SPAR files
   (next to the netlist that names them first, then the working directory). The run manifest's `data_files`
   keep "the path used to open". Check that every existing golden case resolves the same file (they use
   working-directory paths); add a case with a value file next to a netlist in a subdirectory. CHANGELOG
   (Changed): a file of the same name next to the netlist now wins.
5. **Remove the legacy CROW testbench** (`src/tb/crow_tb.cpp`, `specs -t ac_crow`): covered by
   `tests/bench/crow_gen.py --preset crow_tb` (PF06 report, "Does the generator cover `crow_tb`?"). Remove the
   testbench, its options (`--nrings_crow` if only it uses them), its golden cases (`tb/ac_crow` and any other
   case that runs `-t ac_crow`: check `tests/golden/cases.json`), the oracle's `--legacy-tb` comparison, and every
   other user (grep the repository: scripts in `discover/`, `utils/`, docs). Where a removed golden case covered
   something no other case covers (e.g. `--vci` on a CROW, many rings), replace it with a netlist case written
   by the generator. Docs and the testbench list (`specs -p`), CHANGELOG (Removed).

## Acceptance
- `pixi run test gcc`, `pixi run test asan-clang`: all OK, no sanitizer message; the run-table test no longer
  skipped in pixi builds.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: only the predicted cases change (new, deleted or
  re-recorded), each as predicted.
- clang and asan-gcc: before/after `compare` (strict) shows the same set.
- `stats-compare` (gcc) before/after: identical except predicted cases.
- Report `backlog/sprint-2026-10c/PK-REPORT.md` committed on `ws/kfixes`: the completion marker.
