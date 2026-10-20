# SPECS review — summary (sprint 2026-10)

In-depth review of the whole repository at `sprint/2026-10` @ a9e5799, done on branch `ws/review`. The index is
[`../backlog.md`](../backlog.md), the schema is in [`README.md`](README.md), and the disposition of every discovery
seed is in [`SEEDS.md`](SEEDS.md).

## How the review was done
- **7 review areas**, one reviewer agent each, every file read in full: (A1) engine, CLI and `src/utils`; (A2) passive
  devices; (A3) sources, sinks, active and composite devices; (A4) parser and netlist language; (A5) tests,
  testbenches, examples and the golden harness; (A6) Python wrapper, helper scripts and docs; (A7) build and CI.
  Opus reviewed A1–A4 and Sonnet reviewed A5–A7. Reviewers reproduced issues on a Release build (`-O3 -DNDEBUG`), and
  used a Debug build for assert behaviour, valgrind, and patched scratch copies to measure golden impact.
- **Dedupe:** the reviewers filed 190 findings. The orchestrator merged 14 duplicates into the issue they
  duplicate (listed in that issue's *Related* section) and split 2 behaviour-neutral parts out of larger issues
  (B-177, B-178), giving 178 issues (B-001…B-178). Overlapping cleanups carry "whichever lands first" notes.
- **Independent second check:** every `easy-unattended` issue and every issue that was `high` when filed went to a
  fresh verifier (8 batches). Each verifier tried to refute the issue. For easy fixes, the verifier applied the
  suggested fix to a scratch copy, rebuilt it, ran the golden check and compared the result with the claimed
  golden impact. Outcomes and the corrections that followed are recorded in each issue's *Verification* section.
- **Verdicts:** 85 second checks: 61 CONFIRMED, 24 ADJUST (the problem was real but a fix, impact, class or line number needed correcting; all corrections were applied to the issue files), 0 REFUTED. Reclassifications after the second check: B-035, B-052, B-104, B-146. By the same rule (a fix that changes results of netlists that run today is not
  `easy-unattended`), the orchestrator also classified B-147 and B-009 as `needs-human` when merging. Severity changes after
  the second check: B-002 and B-047 lowered from high to med.

## Counts
Total: 178 issues

| severity | easy-unattended | needs-design | needs-human | total |
|---|---|---|---|---|
| critical | 0 | 0 | 0 | 0 |
| high | 4 | 5 | 6 | 15 |
| med | 14 | 27 | 10 | 51 |
| low | 52 | 52 | 8 | 112 |
| **total** | **70** | **84** | **24** | **178** |

| category | critical | high | med | low | total |
|---|---|---|---|---|---|
| style/maintainability | 0 | 0 | 0 | 31 | 31 |
| bug | 0 | 8 | 12 | 10 | 30 |
| parser | 0 | 1 | 9 | 11 | 21 |
| docs | 0 | 0 | 5 | 11 | 16 |
| python | 0 | 2 | 2 | 10 | 14 |
| physics | 0 | 2 | 6 | 5 | 13 |
| build | 0 | 1 | 1 | 11 | 13 |
| test-gap | 0 | 0 | 5 | 8 | 13 |
| cli | 0 | 0 | 3 | 8 | 11 |
| memory/UB | 0 | 0 | 3 | 3 | 6 |
| perf | 0 | 1 | 2 | 2 | 5 |
| determinism | 0 | 0 | 3 | 2 | 5 |

Effort: {'XS': 80, 'S': 76, 'M': 22}  Verdict: {'CONFIRMED': 175, 'PLAUSIBLE': 3}

easy-unattended by effort: {'XS': 54, 'S': 16}

## Headline
- **Nothing is `critical`.** The shipped examples that run (`add_drop*.cir`, `subckt_tests.cir`) give correct
  results: `add_drop.cir` matches the closed-form add-drop transfer function to 1.4e-6 W (B-137). The serious bugs
  are on less-travelled paths:
  - bidirectional (`&`) nets;
  - subcircuits that share nets;
  - multi-wavelength `.op`/`.dc`;
  - positional element arguments;
  - photodetectors;
  - probes in `.tran` after the operating point;
  - several half-finished CLI options.
  Most of them fail **silently**: the run completes with a wrong number, and nothing on stdout says so.
- **The netlist-elaboration layer is the weakest part of the code.** Whether light reaches a device can depend on
  the *line order* of the netlist (B-144, B-145, B-147). Two subcircuit instances that share a net either abort or
  silently keep only the last write (B-146).
- **Several fixes that look easy change results**, so they need a sign-off even though the patch is mechanical:
  B-052, B-146, B-147, B-035. The second check moved them out of `easy-unattended` after showing that netlists
  which run today would produce different numbers.

## Top 10
| # | Issue(s) | Why it matters | Class |
|---|---|---|---|
| 1 | **B-144, B-145** (+ B-147) | Bidirectional-net slot allocation: a uni writer placed before a bidir device takes the device's slot, and `upgrade_signal` can disconnect a reader. Light silently fails to arrive (0 W), and the result depends on **netlist line order**. The crossing-output fix B-147 only works together with B-144. | needs-design |
| 2 | **B-146** | Subcircuit flatten merges port nets against the wrong map. Two instances sharing a net abort with E115, or run with last-write-wins; the type check is lost. Mechanical 2-token fix, but it changes results of netlists that run today. | needs-human (sign-off) |
| 3 | **B-001** | Multi-wavelength `.op`/`.dc`: a stale partial value of one λ can be emitted as the converged result (0.25 W instead of 0.5625 W in the repro). No error is reported. | needs-human |
| 4 | **B-048** | Probes are disabled during the OP and never re-sample when `.tran` re-enables them, so every constant (CW) path reads **0 in the VCD** while stdout shows the right value. | needs-design |
| 5 | **B-052** | CWSRC/VLSRC/EVLSRC positional arguments are shifted by one: `cwsrc1 a 1.3e-6 0.5` gives λ = 0.5 m and P = 0. PCMCELL Tc/Ta are blocked by an assert in Debug. | needs-human (sign-off) |
| 6 | **B-032** (+ B-033, B-159) | The crossing creates energy: 2 W in gives 2.42 W out with `xtalk=-20`, and `xtalk=+15` gives 31.6 W from 1 W. It also has no per-wavelength memory, and its bidirectional variant is never instantiated. | needs-human |
| 7 | **B-049, B-050** | The photodetector never drives its electrical output net, and two PDETs keep an open-ended `.tran` alive forever (292 MB of VCD in 20 s). | needs-design |
| 8 | **B-031** | Every `MZI` on a bidirectional net aborts at elaboration (double bind), and the arms are miswired. The fix is verified to match the unidirectional MZI and to be reciprocal. | **easy-unattended** |
| 9 | **B-123** (+ B-125) | `unit_tests` does not compile, so the default `all` target and `install.sh` fail. `angleIsEqual` is wrong even once it compiles, and the fork harness can deadlock. | **easy-unattended** |
| 10 | **B-124** (+ B-021, B-014) | Accuracy semantics: with the default `reltol=1e-4` a lossless ring settles at 0.995 W (error about 50× reltol). The abstol AND reltol gate and the 10·abstol zero-snap are undocumented, and sub-tick delays are silently rounded to zero. | needs-human |

Near misses:
- B-051: PCMCELL is fully opaque by default (Tc = Ta = 0).
- B-002 and B-004: `-m sampled-time` silently drops every optical write; analyses silently override `-m`.
- B-076 and B-077: pyspecs never detects a `.dc` sweep and crashes on pandas 3.

## Themes
1. **Netlist plumbing**:
   - bidir slot convention (B-144, B-145, B-147);
   - flatten (B-146, B-151, B-152);
   - driver validation (B-164, B-013 `SC_MANY_WRITERS`);
   - element creation order (B-176, B-016);
   - elaboration cost (B-158).

   Decide the slot convention once and add golden cases for bidirectional nets: today none exist.
2. **Multi-wavelength (WDM) semantics**: FD queue merge (B-001), DC dark points (B-003), the exact-`==` λ registry
   (B-010), the OP printout (B-011), PROBE on WDM nets (B-057), MLPROBE (B-062) and the crossing (B-033). Single-λ
   paths are fine; WDM paths need a design pass and tests.
3. **The VCD is not always what was simulated**: probes after the OP (B-048), the missing final timestep (B-005),
   uninitialised traced members (B-054), the undriven PDET output (B-049) and the dark-detector noise (B-056).
4. **Input validation and error handling**:
   - user-input `assert`s are compiled out in Release (B-148, B-018, B-008, B-153);
   - several inputs crash (B-006, B-149, B-152, B-036);
   - out-of-range parameters give silent garbage (B-034, B-035, B-063);
   - error locations are wrong (B-155) and exit codes inconsistent (B-172).
5. **Accuracy knobs need documented semantics**: tolerances (B-021, B-124), delay quantisation (B-014) and sweep
   endpoints (B-009). They are all `needs-human`, and the paper's accuracy claims depend on them.
6. **Device-model questions for a physicist**:
   - crossing (B-032, B-033);
   - PCM (B-051, B-060);
   - detector (B-059, B-056, B-055);
   - coupler `k` semantics (B-037);
   - hard-coded λ0 (B-039);
   - Clements phase screen (B-070);
   - CROW benchmark parameters (B-071);
   - the disabled GenericWaveguide (B-040).
7. **Half-finished features**:
   - `-m sampled-time` (B-002) and `-m` overridden (B-004);
   - `--mon` (B-058), `-T` and `--nrings_crow` (B-025, B-131);
   - `.nodeset`, `.ic` and `.save` (B-162, B-161, B-157);
   - unreachable OCTANE, GTD and Ring code (B-068, B-069).
8. **The test safety net has holes**:
   - unit tests don't build (B-123);
   - the fork harness (B-125);
   - golden `check` accepts reorders, missing cases, loose print slack and exact signal IDs (B-126 to B-129);
   - `specs -t` always exits 0 (B-019);
   - no closed-form physics tests (B-136, B-137);
   - the baseline is bit-exact only for `-march=native` (B-108).
9. **Docs and Python lag the code**:
   - the cheatsheet is wrong (B-080);
   - the KiCad library is broken (B-081, B-082);
   - there are no build or usage docs (B-084);
   - the paper's Fig. 1b does not parse (B-157);
   - pyspecs bugs (B-076 to B-079).
10. **Cleanup** (about 30 `style/maintainability` issues): dead code, debug prints, copy-paste. Most are
    `easy-unattended` and feed the cleanup phase.

## Recommended order of attack
**Step 0: harden the safety net first.** Cheap human decisions on the test harness only:
- **B-129**: normalise the printed `[NNNN]` signal IDs in golden stdout. Otherwise any refactor that changes the
  number of `OpticalSignal` copies fails 25 baselines with identical values; this blocks B-022 and B-042.
- **B-126**: make `--strict-order` the default, with per-case `unordered_streams` for the two cases whose order
  depends on heap layout (`cir/subckt_tests`, `opt/vci`; root cause B-016).
- **B-127**: missing cases and a skipped gtest should fail `check` in CI.
- **B-128**: tighten the print slack for tolerance and config lines.

**Step 1: unattended easy fixes (phase 2).** Suggested order. Each issue is one commit with a golden check; the issues
listed further below change golden outputs and must re-record the baseline in the same commit.
1. Test infrastructure: B-123, then B-125, B-142, B-136, B-138.
2. Crash and abort fixes: B-031, B-047, B-006, B-008, B-149, B-150, B-036, B-020, B-156.
3. Input validation: B-148, B-153, B-018, B-034, B-165, B-161, B-017, B-064, B-131.
4. Output correctness with a predictable golden change: B-154 (JSON writers), B-054 (detector init), B-132
   (`subckt_test.cir`).
5. Python and docs:
   - B-076, B-077, B-085 to B-092, B-097, B-099;
   - B-082 (CRLF file: use line-targeted edits);
   - B-083, B-100, B-102;
   - **B-094 last** among the pyspecs issues.
6. Build warnings: B-109, B-110, B-111, B-112, B-075, B-177, B-114. B-117 and B-118 only if CMake still exists
   after the Meson merge.

**Step 2: cleanup (phase 3).** B-026, B-027, B-028, B-041, B-043, B-044, B-045, B-065, B-067, then B-068 before B-073,
B-072, B-141, B-173, B-174, B-170, and B-178 after B-047.

**Step 3: human decisions, by value.**
1. One slot convention for bidirectional nets: B-144, B-145 and B-147 together (prototype in
   `repro/v7-bidir-slots-prototype.diff`). Then sign off B-146 and B-052; both are mechanical.
2. B-048 (probe sampling on enable), then B-049 and B-050 (detector output and sampling loop).
3. B-001 (FD queue merge per λ) and the WDM theme.
4. Physics sign-offs: B-032, B-033 and B-159 (crossing); B-051 and B-060 (PCM); B-021, B-124 and B-014 (document
   or change the tolerance and delay semantics); B-037 (`k`).
5. CLI: B-002 (reject `sampled-time` until it is implemented), B-004 (`-m` semantics), B-058 (`--mon`).
6. Docs: B-080 (regenerate the cheatsheet from the grammar), B-084, B-157.

## Ordering constraints and overlaps (for unattended fixers)
- **B-147 must land together with or after B-144.** Applied alone, it breaks circuits that work today.
- B-052 converts its own four positional-argument asserts. B-148 skips them, so the two issues are independent.
- B-123 must come before B-136 and B-142, which need compiling unit tests.
- B-047 must come before B-178: the `__modname` names are only exercised once `tb/mesh` runs.
- B-068 must come before B-073: they share lines in `probe.cpp`.
- **B-063 and B-135 are coupled:** `circuits/subckt_tests.cir` relies on a negative VLSRC power today.
- **Shared lines** (whichever lands first, the other skips the shared lines):

  | Shared lines | Issues |
  |---|---|
  | `crow.cpp:96` | B-027, B-072 |
  | `generic_2x2_coupler.h:35` | B-045, B-111 |
  | tb unused variables | B-111, B-141 |
  | `parse_element.cpp:1058, :1247` | B-065, B-173 |
  | `main.cpp` dead code and the `ParserState` leak | B-026, B-170, B-174 |
  | coupler and CW comments in `parse_element.cpp` | B-045, B-052, B-173 |
- **Meson:** B-117, B-118, B-119, B-120 and B-121 are obsoleted by the Meson migration. B-116 is a checklist for the
  Meson port. B-068 must also drop `ring.cpp` from `src/devices/meson.build` if Meson lands first.
- **`easy-unattended` issues whose golden output changes.** Re-record the named cases in the same commit; the exact
  diffs are in each issue's *Golden impact* section.

  | Issue | Golden cases that change |
  |---|---|
  | B-023 | stdout label `V.m⁻¹` → `√W` in 25 cases (listed in the issue) |
  | B-026 | `opt/tol-resolution-overrides` (debug line removed); also `cir/subckt_tests` and `opt/vci` reorder if the `ParserState` leak is fixed (heap layout) |
  | B-027 | no value change; `cir/subckt_tests` and `opt/vci` reorder (heap layout, B-016) |
  | B-170 | no value change; 6 cases reorder (heap layout, B-016/B-176) |
  | B-047 | `tb/mesh`: SIGSEGV → exit 0 with a VCD (drop `expect: crash`) |
  | B-064 | `cir/paul_3ring_puf` stderr names the missing file |
  | B-065 | debug lines removed from the stdout of 7 cases (8 once B-047 lands) |
  | B-123, B-136 | gtest only: `Waveguide.OperatingPoint` FAIL → PASS; 6 new tests |
  | B-132 | `cir/subckt_test`: parse error → exit 0, no VCD (drop `expect: fail`) |
  | B-138 | one new golden case |
  | B-154 | `cir-json/*`: 10 `writers` values |
  | B-173 | `created ROOT/PROBEn` lines removed from the stdout of 4 cases |

## Discovery seeds
All 148 seeds (`S<report>.<n>`, see `README.md`) are accounted for in [`SEEDS.md`](SEEDS.md): 147 became (part of) an issue, 1 were rejected outright; several more were partly rejected (sub-claims below).

**Rejected seeds:**
- **S1.16** (partly rejected): -> B-010 (perf part rejected: registry lookups only at setup and per DC point)
- **S1.24** (partly rejected): -> B-008 ("other sweeps ignored" is wrong: parser.y:405-409 rejects a second sweep)
- **S1.32** (partly rejected): -> B-030 (stack-memory part rejected: lazily committed stacks, small RSS measured)
- **S2.12** (partly rejected): -> B-060. N = 0 is already rejected by the parser; only the C++ constructor default is exposed.
- **S2.23** (rejected): REJECTED. This is a request for a central technology/defaults file, not a defect. Each composite's defaults are explicit, and the waveguide/MZIActive defaults belong to A2.
- **S3.15** (partly rejected): -> B-076 (b01), B-077 (fillna), B-078 (has_wl/stale df); the suggested `has_wl = True` flip is REJECTED as a fix: with an unpowered first probe it makes the wavelength column all NaN (see B-078)
- **S3.18** (partly rejected): -> B-081, B-082; sub-claim "KiCad default net names `Net-(...)` fail to lex" REJECTED: KiCad 7's SPICE exporter writes `Net-_COUPLER2-Pad1_` (parentheses replaced; shown in the external manual's exported netlist), which parses fine (`kicad_manual.cir` equivalent run with `specs -n`: rc 0; only a hand-typed `Net-(WG1-Pad2)` fails); sub-claim "`*_SOURCE_FILE` templates use `values=\`" REJECTED: the 
- **S5.29** (partly rejected): -> B-107 (.mailmap); "local dev ahead of origin/dev, master lacks SystemC 3.0.1" REJECTED: repository state, not a defect in any file

**Seeds kept but with the claim corrected** (see `SEEDS.md` for the full notes):
- S1.01: `--mon` dies right after initialisation and shifts time; it does not kill runs at 0.5 s (B-058).
- S1.17: rejected by the A1 reviewer as unreachable, reinstated after the second check showed that CROSSING reaches
  the cross-λ `+=` branches (B-033).
- S1.12: closing the VCD alone does not fix it; the lost final timestep is the real issue (B-005).
- S2.05: the `k` semantics did not change since the paper; Fig. 1c matches field coupling. Downgraded to docs (B-037).
- S4.02: the first free is done by `MZI::init` via `MeshCol::init`, not by `Clements::init` (B-047).
- S4.10: restoring the PDET write alone does not make `detector_tb` pass, because it never enables its detectors (B-049, B-130).
- S4.15: the `-Wmaybe-uninitialized` warning is a false positive; it is filed only as a warning fix (B-075).
