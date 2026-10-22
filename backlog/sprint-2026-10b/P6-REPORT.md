# P6 report — F-05 outputs and tracing (branch `ws/outputs`)

Sprint 2026-10b, phase P6, on top of `sprint/2026-10b` @ 43a50d7 (P1–P5 merged). Prompt:
`discover/orchestration/2026-10b/P6-outputs.md`. Everything decided for F-05 is **done**: trace only the
probes/`.save` targets when present (trace-all as the fallback), `.save`, `--no-trace`, CSV and `.npz` export. VCD
stays the default format. Nothing was pushed and nothing is held back for sign-off. No simulation result changed:
the only re-recorded outputs are the predicted ones (the automatic probes leave the VCDs of netlists with
probes; the help text gains two flags; the gtest list gains 5 tests).

How the work was split: the orchestrator did the C++ (parser, probes, table writer, CLI), all golden recording and
the bookkeeping. A Sonnet subagent wrote the golden harness's `.npz` decoder and its tests in `/tmp/P6-wt/harness`
(`P6-wip/harness` 45b069b); the orchestrator reviewed the diff and squashed it into the item-4 commit.

## Items

| # | item | outcome | commit | tests | golden impact |
|---|---|---|---|---|---|
| 1 | trace-all fallback | done: after elaboration, `ParseTree::build_circuit()` sets `trace_all_optical_nets = 0` when the flattened netlist has a PROBE or MLPROBE (also inside a subcircuit) or a `.save`, unless `.options traceall` is given explicitly (`trace_all_optical_nets_explicit`; `traceall=0|1` still forces either). A netlist without probes traces every net as before | fd381fc (+ `.save` part in 9feac83) | new `regress/f05-mlprobe-in-subckt`; `regress/b048-probe-cw-tran` (explicit `traceall=1` + PROBE) unchanged; `cir/add_drop` (no probe) unchanged | **26 regress cases re-recorded, as predicted**: `backlog/repro/p6-verify-traceall.py` checks each against the old baseline: the VCD loses exactly the `SystemC.PROBE{...}` scopes, `trace_all_optical_nets` 1 → 0, stdout only `- trace all optical nets: 1` → `0`, and every other signal (explicit probes, PDETs: 8 to 24 per case) is bit-identical; stderr, exit status and manifests identical. 15 have a VCD, 11 are `.op` runs without one (config echo only). Baseline 3.0 → 2.2 MB |
| 2 | `.save` | done: `.save target ...`, a target = a net (power, abs, phase, wavelength, like PROBE without arguments) or `QTY(net)` with the PROBE keywords (`P/POW/POWER`, `E/MOD/MODULUS/MAG/MAGNITUDE`, `PHI/PHASE`, `WL/WAVELENGTH`). One probe per signal of the net (two on a bidirectional net), named like the automatic probes `PROBE{<signal>}`; targets on one net add up. Errors: unknown quantity = parse error with file:line; unknown net, electrical net = error after elaboration, exit 1. `--json` lists the targets | 9feac83 | 6 golden cases: `regress/f05-save-{paper-fig1b,tran,traceall-bidir}`, `err/f05-save-{unknown-net,electrical-net,unknown-quantity}`. Every saved signal is bit-identical to the old automatic probe of the same net (pre-F-05 binary, netlist without `.save`; `backlog/repro/p6-vcd-subset.py`: 2, 6, 18 signals) | new cases only |
| 3 | `--no-trace` | done: as `-o ""` (no VCD, no table, no manifest file; `--print-manifest` still prints, `"trace_file": null`); wins over `-o`, prints `Tracing disabled (--no-trace)` | 02a05de | `regress/f05-no-trace` | `cli/help`, `cli/no-args` (3 help lines) |
| 4 | CSV / `.npz` export | done: `--format vcd\|csv\|npz` (repeatable, default `vcd`; exactly the listed formats) writes `<output>.csv` / `.npz`: one row per (time step, probe, wavelength), columns `time_s, signal, wavelength_m, power_W, field_re, field_im`, exact (shortest round-trip); `.npz` = stored zip of one `.npy` per column + `signal_names`, dependency-free writer (`src/trace_table.{h,cpp}`). PROBE, MLPROBE, automatic and `.save` probes feed it. Column layout documented in `doc/netlist.md` "Tracing and output files". Harness: `.npz` outputs decoded to text (one value per line), compared numerically, raw hash kept in `case.json` | 824702e | gtest `TraceTable.*` (CRC-32 known answer, npy header alignment, CSV rows/coalescing/quoting, npz zip structure and CRC, probes writing without a VCD); harness `Npz` tests (44 harness tests OK); 6 golden cases `regress/f05-format-{csv-npz,vcd-csv-dc,npz-mlprobe-pdet}`, `err/f05-format-{invalid,no-trace,unwritable}`; oracle `backlog/repro/p6-table-vs-vcd.py` | `cli/help`, `cli/no-args` (`--format` help), gtest (5 new tests); new cases otherwise |
| 5 | bench, docs, backlog, report | done: CROW benchmark below; `doc/netlist.md` (`.save`, `traceall`, `--format`, `--no-trace`, "Tracing and output files"), README, CHANGELOG, golden README (189 cases); backlog B-157 (partly fixed), notes on B-057 and B-062 | this commit | — | — |

### Oracle for the tables (item 4)
`backlog/repro/p6-table-vs-vcd.py` runs on a `--format vcd --format csv --format npz` run and checks: rows in time
order, at most one per (time step, probe, wavelength); at every row time the VCD value equals the table value
rounded to 16 digits (power and wavelength exactly; abs and phase recomputed from the field within 8 ulp); every
VCD record of a probe has a row; the CSV and the `.npz` hold the same rows bit for bit. OK on 9 netlists: the
paper's Fig. 1b `.dc` (402 rows), CROW-3 (157336 rows, 17 probes), `circuits/add_drop.cir` (automatic probes), two
wavelengths through a crossing, MLPROBE, bidirectional nets, VLSRC steps, `.save` transient, MLPROBE in a
subcircuit. On all of them the VCD written with `--format vcd --format csv --format npz` is identical to the VCD
written without `--format` (`$date`/`$version` stripped).

## CROW benchmark: full tracing vs probes only

`backlog/repro/p6-bench-crow.py` (raw: `P6-bench-crow30.json`): the paper's CROW from
`discover/scout/bench/crow_gen.py`, N = 30 rings, seed 1, 10 × 1 ns pulses, `.tran 20n`, 100 fs, PROBE1 on `through`
and PROBE2 on `drop`, **no `.options traceall`** (so the F-05 rule decides), `-o traces/crow`. Meson release build
(gcc 11, `-O3 -march=native`, asserts on) of 43a50d7 ("old") and of 824702e ("new"); median user time of 3 runs
(i5-6600K, shared machine; the 3 runs agree within 3 %).

| configuration | user s | output | probed waveforms vs old |
|---|---|---|---|
| old binary, default = full tracing (every net auto-probed) | 19.45 | VCD 1003.3 MB | reference |
| new binary, explicit `.options traceall=1` (full tracing) | 19.63 | VCD 1003.3 MB | identical |
| **new binary, default = probes only** | **5.28** | **VCD 15.4 MB** | **identical** |
| new binary, probes only, `--format csv` (no VCD) | 4.88 | CSV 17.0 MB | — |
| new binary, probes only, `--format npz` (no VCD) | 4.90 | npz 7.4 MB | — |
| new binary, `--no-trace` (engine only) | 4.75 | — | — |

The same netlist runs **3.7× faster with 65× less output** by default (19.45 s → 5.28 s, 1003 MB → 15.4 MB), with the
two probed waveforms identical record for record (streamed comparison of the `ROOT/PROBE1|2` variables). Explicit
full tracing costs what it did (19.63 s vs 19.45 s, noise). Probes only is within 11 % of no tracing with a VCD
and within 3 % with a CSV or `.npz` table (the scout's target was 10 % of case C): with two probes the remaining
cost is the VCD writer. The `.npz` is half the size of the CSV and of the VCD.

## Decisions taken (not covered by the decision record; none changes simulation results)
1. **An explicit `.options traceall` wins** (0 or 1); only its absence triggers "trace only the probes". This keeps
   every netlist that sets it unchanged (`add_drop_pyspecs.cir`, the PDET cases, `b048-*`).
2. **"PROBEs" = PROBE and MLPROBE** elements, also inside subcircuits (after flattening). PDET and power meters do
   not count (they are traced in the VCD as before, whatever the rule says).
3. **Netlists only**: the rule, `--no-trace` and `--format` act on `-f` runs; `-t` testbenches set their own
   trace files and `trace_all_optical_nets` (unchanged). C++ users keep the `trace_all_optical_nets = 1` default.
4. **`.save` probe naming** = automatic probe naming (`PROBE{ROOT/out}`, `PROBE{ROOT/a:0}` and `:1` on a
   bidirectional net), so `.save out` gives exactly the VCD variables `traceall` gave for that net. With an explicit
   `traceall=1`, the saved net's probe traces what `.save` lists (it replaces that net's automatic probe).
5. **`.save` scope**: top-level nets only (subcircuit-internal nets cannot be named by the net grammar); a `.save`
   inside a subcircuit body is ignored with the parser's existing warning (`found a directive in subcircuit,
   ignoring.`), like every directive. No `.save all` (it would clash with a net named `all`). Quantity keywords
   are PROBE's; `I(pdet)` and other quantities are a parse error.
6. **`--no-trace` = `-o ""`**: no run manifest file either (the manifest is written next to a trace file);
   `--no-trace` with `--format` is an error; `--no-trace` with `-o` is accepted (no-trace wins).
7. **Table rows coalesced per time step**: one row per (time step, probe, wavelength) holding the last sample of
   that wavelength in the step. The first version wrote every probe sample, including every delta-cycle iterate of
   the `.op`/`.dc` fixed point: 128082 rows for the 201-point Fig. 1b sweep instead of 402. Delta cycles are not
   physical time and the VCD only shows the end of a step too; per-λ rows keep what the VCD loses on WDM nets
   (B-057).
8. **Columns**: `time_s, signal, wavelength_m, power_W, field_re, field_im` (the decision: "time, per-λ power and
   complex field"); no `abs`/`phase` columns (derivable). `.save` quantities only select VCD variables; the tables
   always have every column. Numbers by `std::to_chars` (shortest round-trip, so exact; the VCD prints 16 digits).
9. **`.npz` layout**: columnar (one array per column, `signal` as an int32 index into `signal_names`), not one
   array per probe (names such as `PROBE{ROOT/out}` make awkward keys); stored (not deflated) zip with fixed
   1980-01-01 dates, so the bytes depend on the data only; kept in memory until the end of the analysis (48 bytes
   per row); above 4 GiB (no ZIP64) it is an error that suggests `--format csv`. Names are ASCII (the lexer's
   identifiers) and are widened byte by byte to UTF-32.
10. **`--format` semantics**: repeatable, exactly the listed formats; `--format csv` alone writes no VCD (that is
    where the speed-up is). Without a VCD, a warning says PDET/power meter readouts are only traced in the VCD.
11. **Manifest**: no schema change; `trace_file` is the VCD, else the first table (`.csv`, then `.npz`).
12. **MLPROBE lookup** by its wavelength list instead of its VCD signals (which only exist with a trace file):
    without a VCD it reported every listed sample as `received unsupported signal`. Stdout-only change; no golden
    case ran an MLPROBE without a VCD.
13. **Golden `.npz` storage**: decoded text in `files/<name>.npz.txt`, compared with the text rules (tolerance,
    print slack); the raw SHA-256 and size are kept in `case.json` but not compared (last bits differ between
    compilers).

## Found, not done
- The paper's Fig. 1b uses `dc1`/`dc2` for couplers: `DC` is not an element prefix (B-157, still open for that).
- VCD hygiene from the scout brief, not decided: the VCD is never closed explicitly (B-005: the final time step is
  missing; `.op` runs write no VCD at all, 11 golden cases), and sources print per event (B-066).
- `.save` cannot name a net inside a subcircuit instance (the net grammar has no hierarchical names), and
  directives in subcircuit bodies are dropped with a warning (`.save`, `.options`).
- The tables do not hold PDET readouts or power meter values (warning when there is no VCD).
- pyspecs still reads VCDs only (brief part 4: `pyspecs.load()` for VCD/CSV/npz, pywellen); not decided. pyspecs is
  not affected by item 1: it reads whatever the VCD holds, and its example netlist sets `traceall=1`.
- A PROBE on a WDM net still traces only the last event in the VCD (B-057); the tables have per-λ rows.
- FST output (brief part 5) not done.
- The `.npz` writer has no ZIP64 (4 GiB limit) and keeps the table in memory.
- `.save` target errors after elaboration have no file:line (the directive does not keep its line).

## Final results
On the report commit (code = 824702e), builds without `CC`/`CXX` except the clang one:
- gcc 11 Meson build `/tmp/P6-build` (`-Dcmake_prefix_path=$HOME/.local/opt/specs-deps-next`): `meson compile`
  clean (only the pre-existing strict-aliasing warnings of `sc_trace(SPECSConfig)`); `meson test`: **12/12 OK**
  (unit, 9 testbench smoke tests, golden-harness, golden); `unit_tests`: **86 tests PASSED** (5 new `TraceTable.*`);
  harness self-tests: 44 OK.
- `run_golden.py check --atol 0`: **190 passed, 0 failed, 0 new, 0 missing** (189 `specs` cases + gtest; 13 new
  F-05 cases since 43a50d7; re-recorded: the 26 probe cases, `cli/help`, `cli/no-args` and the gtest entry). New cases recorded with
  `--repeat 2` (deterministic).
- clang 19 Meson build `/tmp/P6-clang` (`CC=clang CXX=clang++`, cross-check only): same golden check with
  `--atol 0`: **190 passed**, including the CSV and decoded `.npz` cases.

Temporary branch left in place: `P6-wip/harness` (45b069b, squashed into 824702e); its worktree was removed.
