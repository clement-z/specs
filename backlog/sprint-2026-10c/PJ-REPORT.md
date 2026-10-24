# PJ — value-list source prints (report)

Branch `ws/vlprints`, from `sprint/2026-10c` at 365d33b. Decision: "Follow-ups (2026-10-23)" in
`backlog/DECISIONS-2026-10-21.md` (VL/EVL source prints: per-sample emission lines and "was enabled" behind `--vci`,
"invalid time" as a warning on stderr). Issue: B-066. Progress and the prediction written before the change:
`PJ-progress.md`.

| Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|
| 1. VLSRC/EVLSRC prints behind `--vci`, one stderr warning for past values | done | baf872b | 6 gtests (`src/test/devices/value_list_source.cpp`) | 37 cases re-recorded: stdout loses 201 lines, nothing else (as predicted in 354c5e5) |
| 2. Golden cases | done | 2fa468a | `regress/b066-vlsrc-vci`, `regress/b066-evlsrc-past` | 2 new cases |
| 3. Docs, CHANGELOG, B-066 | done | 7168885 | — | — |

Bookkeeping: 354c5e5 (progress, prediction), cd5e94a (progress).

## 1. The change
`VLSource` and `EVLSource` (`src/devices/{value_list_source,electrical_value_list_source}.{h,cpp}`):
- Only printed with `--vci` (`specsGlobalConfig.verbose_component_initialization`, set before parsing, so it also
  covers the value-file reader), on stdout as before: "`<name> was enabled`", "`@<t>, <name> emitted: <value>`"
  (also the initial 0 at the enable), the values parsed from a value file (`<t>: <P>W @<wl>nm`, `<t>: <V>V`) and the
  "`Skipping empty/commented/header line`" notes.
- Values in the past: one warning per source on stderr, always printed, with the count:
  `Warning: ROOT/EVLSRC1: invalid time for signal emission (in the past) for 2 of 4 values, not emitted`.
  It replaces VLSource's stdout line per value (EVLSource skipped them silently).
- The emitted values, their times and which values are skipped are unchanged. Each source keeps its own test,
  now in one static `isPast()` used by both the emission loop and the count: an `sc_time` comparison for VLSource,
  seconds for EVLSource.

## 2. Golden impact and proof
Predicted before the change (354c5e5, `PJ-progress.md`, case by case): 37 cases, 201 stdout lines (146 emission
lines, 49 "was enabled", 6 value-file lines). Result, with `prove.py` (a script outside the repo):
- for every case, the baseline stdout minus the predicted lines equals the new stdout;
- every other stored file (stderr, VCD/CSV/NPZ traces, manifests, `case.json`) is byte-identical;
- the changed cases are exactly the predicted list, and they were the only failures of the strict check before the
  re-record.

The same proof, two recordings of the full case list each (before = 354c5e5, after = this branch), holds for
**gcc release, clang release, asan-clang and asan-gcc**: 37 stdouts, 201 lines, no other difference.
`--stats` counters (`stats-record`/`stats-compare`, gcc): 239 same (141 with statistics), 0 differ.

The re-record (baf872b, one `rerecords` entry: the 37 ids + `gtest/unit_tests`) and the 2 new cases (2fa468a) are
in separate commits. The 6 new gtests were added to the gtest baseline. `tests/golden/README.md`: 241 cases,
`regress/*` 110.

## Decisions taken (unattended, conservative)
1. **Photodetector "was enabled" not touched.** PG-REPORT counted 77 "was enabled" lines in 41 cases, but 27 of them
   come from `Detector` (`ROOT/PDET*`). The decision and the phase prompt name only VL/EVL, so the PDET lines
   stay on stdout. Hence 49 lines in 36 cases.
2. **The EVLSRC value-file listing is behind `--vci` too.** `regress/b052-evlsrc-positional` loses 3 parse lines that
   the prompt's count (only `b052-vlsrc-positional`) did not list. The EVLSource reader is a copy of VLSource's
   (B-074), and the decision names both sources. This was predicted before the change.
3. **`opt/vci` does not change**: it runs with `--vci`, so it keeps its lines (37 changed cases, not 38).
4. **EVLSource gets the warning as well.** Before, it skipped past values silently. Its test (in seconds) is kept.
5. **When the warning is printed:** at the enable, from a count done with the loop's own test. Time only advances in
   the loop's waits (to exactly `sc_time(t)` of each emitted value), so the count is exact. The warning is also
   printed when the run ends before the last value, and it is one line even in EVLSource's case of sub-resolution
   values skipped after a round-up. Printing it at the end of the list would miss runs that end first, and
   printing it before each wait could print two lines.
6. **Warning text** keeps the old phrase so it can still be found:
   `Warning: <name>: invalid time for signal emission (in the past) for <n> of <N> values, not emitted`.
7. **Golden warning case uses EVLSRC.** From a netlist, VLSRC cannot reach the warning: sources are enabled at
   t = 0 and a negative time wraps (see below). The VLSource warning is tested by a gtest that enables the source
   at 2.5 ps.
8. Docs: VLSRC says "Times must be >= 0" (see the first item below), rather than claiming a warning for negative
   times.

## Found, not done
- **VLSRC with a negative time stops the source (new issue, not in the backlog).** `sc_time(double)` truncates
  `v*scale + 0.5` to `int64` and stores it as `uint64` (`sc_time.cpp:154-163`), so -1e-10 s becomes a huge time.
  The value is not "in the past"; the source waits for it forever and emits nothing after it. The initial 0 is
  emitted because `.value() > 0`. No warning; it was the same before this phase. EVLSRC compares in seconds and
  skips it (now with the warning). A small fix would reject negative times at parse time, or use EVLSource's test,
  but that changes behaviour, so it needs a decision.
- **VLSource and EVLSource still differ in their "past" test** (`sc_time` vs seconds; B-074, the copy). In EVLSource,
  a value less than one resolution step after a value that rounded up is skipped (now reported in the warning).
- **B-066 remains partly open**: the CWSRC "`<name> emitted: …`" line per point (out of scope by the prompt) and the
  PDET "was enabled".
- `regress/b144-three-writers` stores a stale stdout (`sc_thread_process` where the binary now prints
  `sc_method_process`, F-15). The case ignores stdout, so it is harmless, but the stored file is misleading.
- The asan-gcc `unit` suite hits meson's 30 s timeout when the host is loaded (load average 18 on 4 cores during
  this phase). The before build (354c5e5) times out the same way. Run directly, 128/128 pass with no sanitizer
  message (30 s wall under that load). Already noted at the F gate. A larger `timeout_multiplier` (or `timeout:`
  in `src/test/meson.build`) for sanitizer builds would avoid it.

## Final results (HEAD of `ws/vlprints`)
- `pixi run test gcc`: 12/12 OK.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 242 passed, 0 failed, 0 new, 0 missing.
- `pixi run test asan-clang`: 12/12 OK, no sanitizer message.
- `pixi run test clang`: 12/12 OK.
- `pixi run test asan-gcc`: 11/12 OK, `unit` TIMEOUT under host load (also before the change; 128/128 pass when run
  directly, see above).
- Before/after proof: gcc, clang, asan-clang, asan-gcc identical apart from the 201 predicted stdout lines; `--stats`
  counters identical.
- Not pushed. Heartbeat 6582c2bb deleted after this commit.
