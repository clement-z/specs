# PD5 report: probes on multi-wavelength nets (PROBE wavelength latch + B-184)

Branch `ws/probes`, from `sprint/2026-10c` (e5d6c21). Prompt `discover/orchestration/2026-10c/PD5-probes.md`;
decisions: "D3 plain probes" and "D5 and v0.3.0" (2026-10-23) in `backlog/DECISIONS-2026-10-21.md`, backlog B-184.
No `git push`, no merge.

**Outcome.** Both items done as decided. The golden impact is exactly the predicted one, written in
`PD5-progress.md` before any code change. B-184 changes no existing case. The latch re-records two cases (same
netlist), and in both each new PROBE trace equals the MLPROBE trace of the latched wavelength from the same run,
at every time point. All other cases are strictly bit-identical (gcc, clang, asan-gcc), and the event counters
are identical in all 228 pre-existing cases. `C-notrace N=30` is not slower (see "Performance").

## Items

| # | Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|---|
| 0 | prediction | done | 7a93ea9 | instrumented base build (not committed) | — |
| 1 | B-184: MLPROBE traces every listed wavelength of the OP at `.tran` start | done | 5f62e0a | gtest `MLambdaProbe.TracesEveryListedWavelengthReceivedWhileDisabledWhenEnabled`; golden `regress/b184-mlprobe-tran-start-multilambda` (the repro, VCD + CSV) | new case only; gtest list |
| 2 | PROBE wavelength latch, `wl=`, warnings | done | 822ef41, 02fb72c | gtest `Probe.Latch*` (3), `Probe.SelectedWavelengthIsTheOnlyOneTraced`, `Probe.TestbenchProbeTracesEveryWavelength`, `TraceTable.DiscardDropsTheRowOfTheCurrentTimeStep`; golden `regress/probe-latch-tran`, `regress/probe-latch-dc`, `err/probe-wl-invalid` | re-recorded `regress/b033-crossing-two-lambda-tran`, `regress/f15-stats-tran` (predicted); 3 new cases; gtest list |
| 3 | docs | done | 468b89a | — | — |
| 4 | checks, bench, report | done | this commit | see "Checks" | — |

### 0. Prediction (7a93ea9)
Two sources of evidence:
- a scan of the baseline outputs: plain-probe `.wavelength` signals and CSV rows that hold more than one wavelength;
- an instrumented base build whose probes keep listening while disabled and report on stderr:
  - several wavelengths in one time step;
  - several wavelengths over the run;
  - the operating-point wavelengths at the enable.

All the differences between the instrumented build and the baseline were in stderr (15 cases). This also showed
that a probe that keeps listening while disabled changes no other output. Findings:
- no `.dc` point carries two wavelengths on a probed net (the multi-wavelength `.dc` cases are WL sweeps);
- no `.tran` operating point gives a plain probe two wavelengths, or an MLPROBE two listed ones;
- only `regress/b033-crossing-two-lambda-tran` and `regress/f15-stats-tran` have plain probes that trace two
  wavelengths;
- the testbenches `tb/det`, `tb/detector` and `tb/ac_add_drop-fd` also do, which led to decision 1 below.

### 1. B-184 (5f62e0a)
- **Option (a)**, local to the probe, as the prompt prefers:
  - while disabled, the MLPROBE keeps listening and stores the last value of each listed wavelength
    (`m_disabled_samples`), without tracing or printing; unlisted wavelengths are ignored silently;
  - when enabled, it traces them in wavelength order (the net's current value is the last one of its
    wavelength), then clears them;
  - the "received unsupported signal" line for an unlisted current value at the enable is kept (B-048 behaviour).
- **Why not (b)** (each output port exposes its `m_emitted_fields`): (a) needs nothing from the ports or the net's
  writer. It sees the net's own values, `.nodeset` and initial conditions included, whatever drives the net. It
  keeps the B-048 sequence, and the plain PROBE latch reuses the same memory. Its cost: during the operating
  point, a disabled probe wakes up on every event of its net.
- **Result** on the repro: 0.5 W, 0.7071 √W at 1550 and 1560 nm from t = 0, in the VCD and the CSV. Before:
  1560 nm read 0, and the CSV had no row for it.

### 2. PROBE wavelength latch (822ef41, warning fix 02fb72c)
- **Latch modes.** `Probe::LatchMode` is set on every `Probe` by the analysis runners: `.op` and `.dc` latch per
  time step (each sweep point is one time step), `.tran` latches once.
- **Rule** (as decided):
  - the smallest wavelength of the analysis point;
  - in `.tran`, the smallest wavelength present when the probe is enabled: while disabled, a latching probe keeps
    the last value of each wavelength, as the MLPROBE does;
  - with none, the smallest of the first time step that brings any. Within that time step, a smaller wavelength
    replaces the latched one, and the replaced sample of that step is dropped from the CSV/npz table
    (`TraceTable::discard()`). The VCD only records the end of a time step;
  - after that, every other wavelength is ignored.
- **`wl=<wavelength>`**: the only wavelength traced, exact match, as for MLPROBE. Decision 2 below covers the clash
  with the existing `wl` flag.
- **Warnings** (stderr, once per run, at the end of the `.dc`/`.tran`, in name order, nothing per event):
  - one line per probe that ignored a wavelength: the probe, its net, the wavelength traced, the first one
    ignored, "use MLPROBE for per-wavelength values";
  - one line for all the `PROBE{<net>}` probes (traceall and `.save`, `Probe::m_automatic`).
- **Warning fix (02fb72c).** The 4-wavelength CROW check showed that the warning named an intermediate latch. A
  VLSRC emits its wavelengths in successive delta cycles of one time step, and the warning reported the latch at
  the first ignore. It now names the wavelength finally latched. The traces were already correct.
- **Proof for the re-recorded cases** (`/tmp/pd5/scripts/probe_vs_mlprobe.py`, piecewise-constant VCD comparison
  at every time point of either signal):
  - in both cases, `PROBE1`/`PROBE2` power, abs and phase equal `MLPROBE1`/`MLPROBE2` at 1550 nm (0 differences);
  - `PROBE2` now traces the 1550 nm crosstalk on `out2`, which arrives at 1 ps, one step before the 1560 nm
    through signal;
  - the golden diff lists only these PROBE signals and the two new warning lines on stderr.

### 3. Docs (468b89a)
- **`doc/netlist.md`:**
  - PROBE syntax in the element table and the PROBE section;
  - "A PROBE traces one wavelength" replaces "PROBE is for single-wavelength nets": `wl=`, the latch per analysis
    point, the `.dc` behaviour of unchanged nets, the warnings, the testbenches;
  - `.tran` start: the probes keep each wavelength's last value while disabled (B-184).
- **`CHANGELOG.md` `[unreleased]`:** New (latch), Fixed (B-184), and the "Faster engine" entry's note about plain
  probes updated.
- **Backlog:** B-184 status "fixed in 5f62e0a" with a fix section; its `backlog.md` row.
- **Cheatsheet:** it is a PDF (`doc/resources/cheatsheet.pdf`) with no source in the repository, so it is not
  updated ("Found, not done").

## Decisions I had to take (conservative)
1. **The latch is set by the netlist analyses** (`runOPAnalysis`/`runDCAnalysis`/`runTRANAnalysis`), not in the
   `Probe` constructor:
   - the legacy `-t` testbenches run none of them, so their probes keep tracing every event (`LATCH_OFF`);
   - `tb/ac_add_drop-fd` sweeps the wavelength over time with a plain probe: a latch would have reduced it to its
     first wavelength;
   - every `tb/*` case is unchanged. C++ users get the latch when they call an analysis runner.
2. **`wl=` clashes with an existing PROBE keyword.** `WL`/`WAVELENGTH` already meant "trace the wavelength
   signal" (boolean). The new meaning applies when the value is a DOUBLE other than 0 and 1 (`1.55e-6`, `1.55u`,
   `{wl0}`). `wl=0|1|true|false` (INTEGER or BOOLEAN, or 0.0/1.0) keep the old meaning; a DOUBLE <= 0 is an error
   (`err/probe-wl-invalid`). Both aliases behave the same. The maintainer may prefer a distinct keyword.
3. **What "present at a `.dc` point" means:** written to the net at that point. A probe cannot know more. In a
   `.dc`:
   - each point resets the output ports and the CW sources re-emit;
   - but a CW source that is not swept writes the same value, so its net gets no event and nothing downstream is
     written again at its wavelength;
   - a fixed 1550 nm source next to a swept one therefore reaches the probe at the first point only;
   - at later points the probe sees only the swept wavelength, as before PD5 (`regress/probe-latch-dc` records
     this, and `doc/netlist.md` says so). A WL sweep (one source) is exact.
4. **`.op`:** set to per-point latching for consistency. The probes are disabled during the `.op` fixed point (as
   before), so a `.op` traces no probe value and the latch has no visible effect.
5. **The latch is reset at each enable** (`.tran`: the enable after the OP), so a latch from the initialization
   run (probes enabled, nothing written in practice) does not carry over.
6. **Warnings are printed at the end of the analysis**, in name order, rather than at the first ignore. The text
   does not depend on the kernel's event order, and nothing is printed per event. The `.save` targets count as
   automatic probes for the summary line, since they are the same `PROBE{<net>}` probes. A `.dc` probe's warning
   names the wavelength traced at the point of the first ignore.
7. **Golden case names** `regress/probe-latch-*`, `err/probe-wl-invalid`: no backlog id for the latch (a decision,
   not an issue).

## Checks
- **B-184 repro** (`backlog/repro/b184-...cir`, the command in its header):
  - base: one CSV row (1550 nm), `power@1560_000000` = 0;
  - final: 0.5 W at both wavelengths from t = 0.
- **4-wavelength CROW** (`crow_gen.py 3 --nwl 4 --pulses 3`, a PROBE and an MLPROBE at the 4 wavelengths on each
  of the 16 nets, gcc):
  - the 15 driven PROBEs latch 1548.8 nm (the smallest) and equal the MLPROBE at 1548.8 nm at every time point
    (45/45 quantity checks, e.g. `drop`: 13705 time points);
  - `add` is undriven: no trace;
  - against base, all 192 MLPROBE signals and the 8 config signals are identical; 56 of 64 PROBE signals change
    (the latch);
  - stdout identical (except the trace file name).
- **`circuits/add_drop.cir`:** base vs final, stdout, stderr and VCD identical (40 probe signals). The WL-sweep
  golden cases (`cir/add_drop*`, `opt/*`, `regress/f12-dc-wl-*`, `regress/f05-*-dc`, `regress/f15-stats-dc`,
  `regress/b004-mode-td-dc`) are strictly identical.
- **Event counters** (`stats-record` base e5d6c21 vs final): 228 same (94 without run statistics), 0 differ, 4 new (the new cases). `regress/f15-stats-tran` (re-recorded) has the same counters: probes have no output ports.
- **clang release, asan-gcc** (strict recordings, base vs final): each build compared with its own base recording. Only `regress/b033-crossing-two-lambda-tran` and `regress/f15-stats-tran` differ, and only in the PROBE1/PROBE2 signals and stderr, as with gcc (226 identical). The 4 new cases and the 2 re-recorded ones pass strict against the gcc baseline with both builds.
- **asan-clang:** `pixi run test asan-clang` 12/12 OK, no sanitizer message in the test log; `tests/sanitizers/lsan.supp` unchanged. Item 1 alone (5f62e0a) was checked the same way: counters 228 same, clang and asan-gcc 228/228 identical to base, asan-clang 12/12.

## Performance (CROW bench `C-notrace N=30`)
Pixi gcc 15 portable release builds of the tip before D5 (e5d6c21, `/tmp/pd5-wt/base`) and of the final tree
(15a36c3, `/tmp/pd5-wt/final`). Interleaved runs on the same netlist, `RUSAGE_CHILDREN` user time as in
`bench_crow.py` (script `/tmp/pd5/scripts/bench.py`). Shared host, load 1.5 to 3.6. All runs simulated 20 ns and
exited 0.

| case | rounds | base (median s) | final (median s) | base/final |
|---|---|---|---|---|
| **`C-notrace N=30`** (`30 --traceall 0 --probes 0`) | 5 | 1.872 (1.908, 1.850, 1.879, 1.863, 1.872) | 1.867 (1.827, 1.875, 1.838, 1.889, 1.867) | **1.003** |
| `A-scaling N=30 probes=2` (`30 --traceall 0 --probes 1`), for information | 3 | 2.225 | 2.249 | 0.989 |
| `D-wdm N=10 nwl=4` (`10 --traceall 0 --probes 1 --nwl 4`), for information | 3 | 8.879 | 8.773 | 1.012 |

`C-notrace N=30` is not slower (1.003x; it has no probes). The cases with probes are within the noise of the host
(±1 %).

## Found, not done
- **`.dc` and unchanged nets** (decision 3): a net whose value does not change between points is not written
  again. A plain probe therefore cannot see a non-swept wavelength after the first point. Rewriting every net at
  each point would change `.dc` event counts and outputs. Needs a decision.
- **`.op` traces nothing:** the probes are disabled for the whole `.op` (pre-existing). MLPROBEs are disabled in
  `.dc` too, so they trace nothing there.
- **`wl` keyword overload** (decision 2): a dedicated keyword (e.g. `lambda=`) would avoid the 0/1 special case.
- **Cheatsheet PDF** (`doc/resources/cheatsheet.pdf`): no source in the repository, PROBE `wl=` not in it.
- **MLPROBE "received unsupported signal"** is still printed on stdout for every event at an unlisted wavelength
  while enabled (pre-existing, PD3 report).
- **Probe threads** now wake on every event of their net during a `.tran` operating point (before: only at the
  enable). This is negligible in the bench (see "Performance") but scales with the number of probes on busy OP
  nets.

## Final results (tip 15a36c3 (code at 02fb72c) + this report)
- `pixi run test gcc`: 12/12 OK.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: 233 passed, 0 failed/new/missing. That is 228 pre-existing `specs` cases (2 re-recorded, as predicted), 4 new cases, and the gtest case (re-recorded for its list: 120 tests).

## Housekeeping
- Heartbeat `20f4caae` deleted after this commit.
- Scratch in `/tmp/pd5`: recordings, counters, CROW netlists, the scripts `refs.sh`, `after.sh`, `vcdscan.py`,
  `probe_vs_mlprobe.py`, `vcdcmp.py`, `bench.py`.
- Worktrees `/tmp/pd5-wt/{base,instr,item1,item2,final}` (detached, local only): not needed for the merge, the
  coordinator can remove them (`git worktree remove`).

## Follow-up (after the gate, maintainer decisions)

Two decisions applied on `ws/probes` as separate commits. They supersede the parts above that describe the PROBE
`wl=<wavelength>` selection (decision 2 of "Decisions I had to take", the `wl=` lines of item 2, the gtest
`Probe.SelectedWavelengthIsTheOnlyOneTraced`, the golden case `err/probe-wl-invalid`). Prediction written first
in `PD5-progress.md` (d145ceb).

| # | Decision | Commit | Golden impact |
|---|---|---|---|
| F1 | No PROBE wavelength selection keyword in v0.3.0: `wl`/`wavelength` keeps only its boolean meaning | 1f44df4 | `err/probe-wl-invalid` removed; gtest list; `regress/probe-latch-tran`, `regress/probe-latch-dc` re-recorded (see below) |
| F2 | The `.dc` limit as a backlog item: B-185 (open, needs-design) | ddd59de | none |

### F1 (1f44df4)
- **Parsing:** PROBE's `wl` / `wavelength` keyword is parsed exactly as on e5d6c21 (`as_boolean()`), so a value
  other than 0/1 means "trace the wavelength". Checked: `wl=-1.55e-6`, `wl=1.56e-6` and `wl=0` give the same exit
  status, stdout and traced variables as e5d6c21.
- **Removed:** `Probe::m_wavelength_m` and its code paths (`accept()`, `latchAtEnable()`), the gtest
  `Probe.SelectedWavelengthIsTheOnlyOneTraced`, and the golden case `err/probe-wl-invalid` (netlist, baseline
  directory, manifest case list; its `rerecords` history entries are kept).
- **Kept:** the latch, its warnings and B-184.
- **Docs:** `doc/netlist.md` says to use MLPROBE for a given wavelength (no selection keyword on PROBE); the
  CHANGELOG entry is reworded.
- **Beyond the coordinator's predicted list** (stated before the change): my two new cases
  `regress/probe-latch-tran` and `regress/probe-latch-dc` each had a `probe2 ... wl=<wavelength>` that only
  exercised the selection. They change whatever is done, because `wl=1.56e-6` is "true" again. I removed
  `probe2` from both netlists and re-recorded only these two cases. Proof (`/tmp/pd5/scripts/fuproof.py`, old vs
  new recording):
  - the VCD, CSV and stderr equal the previous ones minus every `PROBE2` signal (4), row and warning line;
  - stdout differs only in the parse listing (PROBE2's line, the element count, the reader count of `m`);
  - the run manifest differs only in the netlist hash.
- **Results** (tip 1f44df4, snapshot `/tmp/pd5-wt/fu`):
  - gcc: `meson test` 12/12; strict golden 232/232 (231 `specs` cases + gtest, 119 tests);
  - counters: 228 same vs e5d6c21, 231 same vs the gated tip (15a36c3);
  - clang and asan-gcc, strict recordings vs the gated tip: 229 identical, only `regress/probe-latch-tran` and
    `regress/probe-latch-dc` differ. Vs e5d6c21: as at the gate, only the two predicted latch re-records. The 5
    new or re-recorded cases are strict against the gcc baseline on both;
  - `pixi run test asan-clang`: 12/12, no sanitizer message.

### F2 (ddd59de)
`backlog/B-185-dc-unchanged-nets-not-rewritten-between-points.md` and its `backlog.md` row (low, bug, M,
needs-design, open). In a `.dc`, a net whose value does not change between points is not written again, so a
probe sees a non-swept wavelength only at the first point. Evidence `regress/probe-latch-dc`, documented in
`doc/netlist.md` (now pointing to B-185); fix options (a) to (c).

### Found, not done (updated)
- The "`wl` keyword overload" item above is resolved by F1.
- The `.dc` limit is now B-185.

## Follow-up 2 (maintainer decision: `.dc` latch on the swept wavelength, option a)

Commit 3152794; prediction written first in `PD5-progress.md` (17b9959).

- **Rule.** In a `.dc` that sweeps a source's wavelength or frequency (`WL(src)`, `F(src)`, `/src/WL`, `/src/F`),
  each plain PROBE latches, at each point, the swept wavelength when its net carries it at that point. Otherwise
  the smallest wavelength present (unchanged). Power sweeps, `.op` and `.tran` are unchanged.
- **Explicit API, no global:**
  - the `.dc` sweep order is a struct (`CWSweepOrder`: setter, values, the swept `CWSource`, `sweeps_wavelength`
    for WL/F) instead of a pair, filled by `DCAnalysis::create()`;
  - before each point, `runDCAnalysis()` reads the swept source's wavelength (after applying the value, the exact
    double the source emits) and calls `Probe::setSweptWavelength()` on every probe (NaN for other sweeps);
  - in `Probe::accept()`, within the point's time step, the swept wavelength replaces any other latched one (its
    table row is dropped) and is then kept for the point.
- **Golden impact, as predicted:**
  - no existing case changes, `regress/probe-latch-dc` included: its first point already latched the swept
    1541 nm, the smallest there next to the fixed 1550 nm. The coordinator expected its first point to change; it
    cannot, since swept and smallest coincide there;
  - the other `.dc` cases have a single source;
  - new cases: `regress/probe-latch-dc-swept-wl` (fixed 1530 nm next to a WL sweep from 1541 to 1559 nm) and
    `regress/probe-latch-dc-swept-f` (the same with `/cwsrc2/F`). At the first point the probe now shows the
    swept wavelength (1541 nm, resp. c/f = 1558.98 nm) where 1f44df4 showed the fixed 1530 nm; the later points
    are the same. A net with two fixed wavelengths (`probe2` of the WL case) keeps the smallest (1520 nm);
  - gtest `Probe.LatchPerTimeStepPrefersTheSweptWavelengthOfADcPoint` added; gtest list re-recorded.
- **Docs:** `doc/netlist.md` (latch rule in `.dc`), CHANGELOG `[unreleased]`; B-185 updated (first-point part
  solved, unchanged-net part still open) and its `backlog.md` row.
- **Results** (tip 3152794, snapshot `/tmp/pd5-wt/fu2`):
  - gcc: `meson test` 12/12; strict golden 234/234;
  - counters: 231 same vs 1f44df4 (+2 new), 228 same vs e5d6c21;
  - clang and asan-gcc, strict recordings vs 1f44df4: 231/231 identical (+2 new). The 7 new or re-recorded PD5
    cases pass strict against the gcc baseline on both;
  - `pixi run test asan-clang`: 12/12, no sanitizer message.
