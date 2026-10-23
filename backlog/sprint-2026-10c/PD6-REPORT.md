# PD6 report: PROBE always traces its wavelength

Branch `ws/probewl`, from `sprint/2026-10c` (a42e0f2). Prompt `discover/orchestration/2026-10c/PD6-probe-wl.md`;
decision "PROBE wavelength (2026-10-23)" in `backlog/DECISIONS-2026-10-21.md`. No `git push`, no merge.

**Outcome.** Done as decided. The golden impact is exactly the one predicted in `PD6-progress.md` before the
code change. Two cases are re-recorded: the two netlists with a false wavelength flag. Each gains its probe's
`wavelength` signal and the warning line, and nothing else changes. Everything else is strictly bit-identical
(gcc, clang, asan-gcc), and the event counters are identical in every case.

## Items

| # | Item | Outcome | Commit | Tests | Golden impact |
|---|---|---|---|---|---|
| 0 | references, prediction | done | b735ef4 | base: gcc 12/12, strict golden 234/234 | — |
| 1 | PROBE / `PROBE{<net>}` always trace `wavelength`; a false flag is ignored with one warning | done | 769a17f | gtest `Probe.NetlistProbeAlwaysTracesTheWavelength`; golden `regress/probe-wl-ignored` | re-recorded `regress/b153-zerotol-lossy-ring-tran`, `regress/probe-latch-tran` (predicted); 1 new case; gtest list |
| 2 | docs (`doc/netlist.md`, element table, CHANGELOG) | done | 30a4a77 | — | — |
| 3 | checks, report | done | this commit | see "Checks" | — |

### 1. Implementation (769a17f)
- **`ProbeElement::create()`** (`src/parser/parse_element.cpp`) always sets `m_trace_wavelength`.
  - The 4th positional argument and `wl`/`wavelength` are still parsed with `as_boolean()`, so invalid
    values give the same errors as before.
  - Their net value is computed as before: a keyword after a false positional overrides it.
  - When that value is false, the probe name goes into `ParseTreeCreationHelper::probes_ignoring_wavelength_off`
    (a `set<string>`, so it is in name order; no global).
- **`ParseTree::build_circuit()`** prints one line for all of them, once, after every element is created:
  `Warning: the wavelength of a PROBE is always traced since v0.3: wl=0 (or a false 4th argument) ignored for ROOT/PROBE1, ROOT/PROBE2, ...`.
  Nothing is printed per event.
- **Automatic probes.** They already traced all four quantities: traceall in `runOPAnalysis()`, and
  `.save <net>` as `ALL`. Not changed. `.save QTY(net)` is not changed either: it keeps exactly the quantity it
  asks for.
- **C++ API.** The `Probe` constructor keeps `trace_wavelength`. Its comment says that the netlist always sets
  it, except for `.save QTY(net)`. The `src/tb/` testbenches are unchanged (all `tb/*` golden cases pass
  strict).
- **Tests.**
  - gtest: keyword, alias `wavelength=0.0`, positional, a keyword that overrides a false positional, a true
    value, and `.save P(a)` (power only, no wavelength). It also checks the exact stderr text.
  - Golden `regress/probe-wl-ignored` adds a probe inside a subcircuit (`ROOT/X1/PROBE1`, listed last in name
    order) and `.save E(b)` (abs only).
- **Re-recorded cases** (strict diff before the re-record; values read from the new VCDs):
  - `regress/b153-zerotol-lossy-ring-tran` (`probe1 out 0 1 0 0`):
    - new `ROOT/PROBE1.wavelength`, 1.55e-6 from t = 0;
    - the PD6 line first on stderr, then the zero-tolerance warning.
  - `regress/probe-latch-tran` (`probe5 c wl=0`):
    - new `ROOT/PROBE5.wavelength`, 0 then 1.54e-6 from 2 ps, the same as `ROOT/PROBE4` on that net;
    - the PD6 line first on stderr, then the 3 latch warnings, unchanged;
    - CSV unchanged.
  - In both cases, stdout and every other VCD signal are unchanged. I updated the netlist comments, which said
    the wavelength was not traced, so the netlist hash in the run manifest changes. I predicted this.

### 2. Docs (30a4a77)
- **`doc/netlist.md`:**
  - element table: `PROBE: [power [abs [phase]]]`; the wavelength is always traced, and a 4th argument or `wl`
    is accepted and ignored;
  - PROBE section: why the wavelength is always traced, what happens to older netlists, and the warning text;
  - `.save`: `QTY(net)` gives exactly that quantity (SPICE semantics), and `WL(net)` adds the wavelength.
- **`CHANGELOG.md` `[unreleased]` / Changed:** one entry.
- **Cheatsheet:** it is a PDF with no source in the repository, so it is not updated (as in PD5).

## Decisions I had to take (conservative)
1. **Warning text and names.** The prompt's example names `PROBE1, PROBE5`. I used the module names
   (`ROOT/PROBE1`, `ROOT/X1/PROBE1`), as the other probe warnings do, and the prefix `Warning: `. The text also
   says "(or a false 4th argument)", so that users of the positional form recognise their case.
2. **When the warning is printed:** at elaboration (`build_circuit()`), not at the end of the analysis. The
   flags are known there, so it is printed exactly once per run, before any analysis output. The run also
   continues normally.
3. **"False" means the net value.** A false positional followed by `wl=1` is a true request, so no warning (the
   old code also ended up tracing the wavelength). The lexer has no `true`/`false` literals: the false forms
   are `0`, `0.0`, or a `.param` or expression that evaluates to 0.
4. **`--vci` prints `trace wavelength: 1`** for such probes: it shows the flag that is actually used. No golden
   case combines `--vci` with a false flag.
5. **Benchmark not run.** The change is parse-time only. The event counters are identical in every case, and
   the bench case `C-notrace` has no probes.

## Found, not done
- **B-078 (pyspecs `KeyError: 'wavelength'` in a sweep without a wavelength trace).**
  - Its repro `backlog/repro/a6-pyspecs-nowl-fd.cir` (`wl=0` on both probes) now traces both wavelengths and
    prints the PD6 warning, so that path no longer triggers it.
  - A `.dc` that saves only `.save P(...)` (traceall off) still has no wavelength signal, so point 3 of B-078
    can still happen.
  - B-078 itself is not updated.
- `tests/golden/README.md` still says "228 `specs` cases" in its case-list heading (already out of date before
  PD6; now 234 + gtest).

## Checks (tip 30a4a77; code at 769a17f, 30a4a77 is docs only)
- `pixi run test gcc`: 12/12.
- `pixi run golden gcc -- --rtol 0 --atol 0 --print-slack 0`: **235 passed**, 0 failed/new/missing (gtest
  121 PASS).
- **Event counters** (`stats-record`/`stats-compare` vs a42e0f2): 233 same, 0 differ, 1 new.
- **clang release, before/after** (strict `compare` of two recordings):
  - only the 2 predicted cases differ, with the same differences as gcc (stderr line, new wavelength
    variable, netlist hash);
  - the 3 PD6 cases pass strict against the gcc baseline.
- **asan-gcc:** same result as clang release.
- `pixi run test clang`, `pixi run test asan-gcc`: 12/12.
- `pixi run test asan-clang`: 12/12, no sanitizer message. `lsan.supp` is unchanged.

No push, no merge.
