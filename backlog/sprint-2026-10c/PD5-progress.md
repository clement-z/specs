# PD5 progress (probes on multi-wavelength nets: PROBE wavelength latch + B-184)

Branch `ws/probes` (from `sprint/2026-10c` e5d6c21). Heartbeat: `20f4caae` (PD5-heartbeat).
Scratch (not in the repo): `/tmp/pd5` (binaries, recordings, counters, scripts); worktrees
`/tmp/pd5-wt/base` (e5d6c21, the reference build) and `/tmp/pd5-wt/instr` (base + stderr instrumentation,
used only for the prediction below).

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 0 | references (base e5d6c21): gcc test + strict golden, clang/asan-gcc recordings, counters, asan-clang; prediction | done | | gcc `meson test` 12/12, strict golden 229/229; recordings `/tmp/pd5/rec/base-{clang,asan-gcc}`; counters `/tmp/pd5/stats/base`; asan-clang 12/12 |
| 1 | B-184: MLPROBE keeps listening while disabled, traces its per-wavelength memory when enabled; repro as a golden case | done | 5f62e0a | option (a); gcc strict 229/229 + new case `regress/b184-mlprobe-tran-start-multilambda` (0.5 W at both wavelengths from t = 0, VCD + CSV), gtest list re-recorded; counters 228 same + 1 new; clang and asan-gcc 228/228 identical to base, new case strict on both; asan-clang 12/12, no sanitizer message |
| 2 | PROBE wavelength latch (`wl=`, per analysis point, warnings) | done | 822ef41, 02fb72c (warning fix) | as predicted: re-recorded `regress/b033-crossing-two-lambda-tran`, `regress/f15-stats-tran` (PROBE1/2 = MLPROBE1/2@1550 at every time point, + stderr warnings, nothing else); new cases `regress/probe-latch-tran`, `regress/probe-latch-dc`, `err/probe-wl-invalid`; gtest +6. At 822ef41: counters 228 same + 4 new; clang/asan-gcc vs base: only the 2 predicted cases differ, new/re-recorded cases strict on both; asan-clang 12/12 clean. `.dc`: a net whose value does not change between points is not written again, so a non-swept CW wavelength reaches the probe at the first point only (documented). Decisions: latch set by the analysis runners (testbenches unchanged); `wl=` takes a DOUBLE other than 0/1, `wl=0|1|true|false` keeps the old flag meaning; warnings at the end of the analysis, name order |
| 3 | docs (netlist.md, cheatsheet, CHANGELOG, B-184, backlog.md) | done | 468b89a | cheatsheet is a PDF (`doc/resources/cheatsheet.pdf`): the element table of `doc/netlist.md` lists the PROBE arguments instead |
| 4 | checks (B-184 repro, 4-λ CROW PROBE vs MLPROBE, add_drop, bench), report | done (PD5-REPORT.md) | | CROW 3 rings 4 λ, PROBE + MLPROBE on all 16 nets: 15 driven PROBEs = MLPROBE@1548.8 nm at every time point (45/45), MLPROBE 192/192 signals identical to base; warning named an intermediate latch (fixed in 02fb72c); add_drop identical; final (15a36c3): gcc test 12/12, strict golden 233/233, counters 228 same + 4 new, clang/asan-gcc only the 2 predicted cases differ, asan-clang 12/12; bench C-notrace N=30 5 rounds 1.003x vs e5d6c21 |

## Predicted golden impact (written before any code change)

Method: (1) scan of the baseline outputs for plain-probe `.wavelength` signals and CSV rows with several
wavelengths; (2) an instrumented base build (`/tmp/pd5-wt/instr`, not committed) whose probes keep listening
while disabled and report on stderr: several wavelengths in one time step (a `.dc` point), several
wavelengths over the run, the operating-point wavelengths at enable (PROBE: more than one, or the smallest is not
the net's current value; MLPROBE: more than one listed wavelength, or the listed one is not the current value).
All its differences from the baseline were in stderr only (15 cases), so listening while disabled changes no
other output.

Findings:
- no `.dc` point has more than one wavelength on a probed net (the `.dc` cases with several wavelengths are
  WL sweeps: `cir/add_drop` and the cases on `circuits/add_drop.cir`, `regress/f12-dc-wl-*`,
  `regress/f05-save-paper-fig1b`, `regress/f05-format-vcd-csv-dc`, `regress/f15-stats-dc`,
  `regress/b004-mode-td-dc`);
- no plain probe sees more than one wavelength during a `.tran` operating point, and no MLPROBE more than one
  listed wavelength (`regress/f05-format-npz-mlprobe-pdet` has two wavelengths on its net, but lists only
  1550 nm, which is also the net's last value);
- in `.tran`, two cases have plain probes that trace two wavelengths: `regress/b033-crossing-two-lambda-tran`
  and `regress/f15-stats-tran` (same netlist: `PROBE1` on `out1`, `PROBE2` on `out2`, 1550 nm from t = 1 ps,
  1560 nm from t = 2 ps, crossing with crosstalk);
- the testbenches `tb/det`, `tb/detector`, `tb/ac_add_drop-fd` have plain probes that see several wavelengths
  (`ac_add_drop-fd` sweeps the wavelength over time). Testbenches run no netlist analysis.

Prediction:
1. **B-184 (item 1):** no existing case changes. New case: the repro
   (`backlog/repro/b184-mlprobe-tran-start-multilambda.cir`).
2. **PROBE latch (item 2):** `regress/b033-crossing-two-lambda-tran` and `regress/f15-stats-tran` change:
   `PROBE1` and `PROBE2` trace 1550 nm only (`PROBE2`: the 1550 nm crosstalk, which arrives first; its 1560 nm
   through signal is ignored), equal to the 1550 nm signals of `MLPROBE1`/`MLPROBE2` of the same run; stderr
   gains the warnings. Event counters unchanged (probes have no output ports).
   Decision taken: the latch is set by the netlist analyses (`.op`, `.dc`, `.tran` runners), so the legacy
   testbenches (`-t ...`), which call none, keep tracing every event: `tb/*` unchanged.
3. WL sweeps (`cir/add_drop*` and the other `.dc` cases above): bit-identical.
4. gtest: the case list (new tests).
5. Every other case: strictly bit-identical, counters identical.

## Follow-up (coordinator, after the gate)

Heartbeat: `abc26e04` (PD5F-heartbeat). Marker: `PD5-FOLLOWUP.md`.

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| F1 | remove the PROBE `wl=<wavelength>` selection (maintainer: no selection keyword in v0.3.0); `wl`/`wavelength` back to the e5d6c21 boolean parsing | todo | | |
| F2 | backlog item for the `.dc` limit (unchanged nets are not rewritten between points) | todo | | |

Predicted golden impact of F1 (written before the change):
- `err/probe-wl-invalid` removed (netlist, case, baseline directory, manifest case list);
- gtest list (`Probe.SelectedWavelengthIsTheOnlyOneTraced` removed);
- **beyond the coordinator's list:** `regress/probe-latch-tran` and `regress/probe-latch-dc`, added in this phase,
  each have a `probe2 ... wl=<wavelength>` that only exercised the selection. With the old parsing `wl=1.56e-6`
  means "trace the wavelength" (true), so these cases change whatever is done. I remove `probe2` from both
  netlists and re-record them. Proof expected: the new outputs equal the old ones minus everything of `PROBE2`
  (VCD signals, CSV rows, its warning line, its parse-listing line and the element count); and the run
  manifest's netlist hash;
- every pre-D5 case (the 228 of e5d6c21) and `regress/b184-*`: strictly identical.
