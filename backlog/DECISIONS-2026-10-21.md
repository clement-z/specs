# Maintainer decisions — 2026-10-21

Recorded from a Q&A with the maintainer after sprint 2026-10 (PR #16). "Approved" items move from
`needs-human`/`needs-design` to scope for sprint 2026-10b; "pass" items stay in the backlog unchanged.

## Correctness and physics
| Item(s) | Decision |
|---|---|
| B-144, B-145, B-147 | **Approved: next-free-slot convention** on bidirectional nets: every writer takes the next free slot, readers read the other side; `upgrade_signal` swaps only when the upgrading device reads the net; crossing outputs bound as writers. One change, starting from `backlog/repro/v7-bidir-slots-prototype.diff`. |
| B-146 | **Approved**: flatten merges subckt port nets against `nets_flat` (hierarchical = flat behaviour). |
| B-052 | **Approved: fix positional order**: CWSRC `wl power phi`, VLSRC/EVLSRC `file`, PCMCELL `emelt n tc ta`; too many args → clean error; document in the cheatsheet. |
| B-001 | **Approved: validated patch** (search the whole queue for the same (t, λ)). Reproduced with `backlog/repro/a1-fd-multilambda-stale.cir`: 0.25 W instead of 0.5625 W at 1550 nm once a 1560 nm signal shares the mergers; single-λ control gives 0.5625 W. Add the repro as a golden case. |
| B-048 | **Approved: robust fix**: on enable, every probe (PROBE, MLPROBE, auto-probes) samples its net's current value, also when it saw no OP event. Maintainer condition: it must work with initial conditions (`.ic`, `.nodeset`) and related flows, i.e. sample after any IC/OP step at TRAN start; add a test that stays valid once `.ic` is implemented. |
| B-049, B-050 | **Approved: event-driven PDET**: write the readout on input events (and on change beyond a tolerance); periodic Ts sampling only while ≥ 2 non-zero wavelengths beat; detectors enabled by default outside `.tran` (testbenches, OCTANE); fix `detector_tb` enable. |
| B-032 | **Approved: passive crossing, `att` = excess loss**: x = 10^(-\|xtalk\|/20) in quadrature (j·x), t = 10^(-att/20)·sqrt(1−x²); positive xtalk read as isolation or rejected (pick one, document). |
| B-033, B-159 | **Approved, together with B-032**: per-λ memories + one thread per input (like the coupler); instantiate `CrossingBi` on bidirectional nets. |
| B-051 | **Approved: require `tc`/`ta`** for PCMCELL (netlist error when missing); C++ constructor defaults aligned with the header; `OctaneCell` passes explicit values. |
| B-060 | **Approved: validation now** (reject N ≤ 0 and k outside [0, N]); model changes deferred to PCM v2 (scout F-10, against measured data). |
| B-037 | **Approved: document only.** Netlist `k` is the field coupling (P_cross = k²); the paper's κ is a power ratio — different parameters, no erratum. Remove the per-instance warning, fix the comments, document k/kp/t. |
| B-002, B-004 | **Approved**: reject `sampled-time` until implemented (at configuration time, not per event); an explicit `-m` overrides the netlist directive's mode (intent of 1b0780d). |
| B-021 | **Approved: keep the rule, document it.** The paper defines tolerances as thresholds below which changes need not be emitted, explicitly not solver tolerances, and its benchmarks used max(abstol, reltol·\|last\|). Keep the rule and the snap-to-0 (it underpins the CROW saturation claim) but expose the snap factor as a documented option (default 10·abstol); make the \|last\| = 0 case explicit. No result change. |
| B-014 | **Approved** with B-021: remove the dead delay check; warn when a delay is rounded by more than a threshold, error when a non-zero delay rounds to 0. |
| B-124 | **Approved: document + warn**: document the feedback error bound ≈ reltol·r/(1−r) and how to choose reltol for resonators; add a tight-reltol golden case. Exact `.op`/`.dc` solve goes to the roadmap. |
| B-058 | **Approved: remove** `TimeMonitor` and `--mon` (progress/stats later via scout F-15). |

## Golden harness
B-126 (strict order by default), B-127 (missing/skipped fail), B-128 (tighter print slack), B-129 (normalise
printed signal IDs): **all approved**, done first.

## Features (scout horizon 1)
| Feature | Decision |
|---|---|
| F-01 reproducible runs | **Approved**: per-device RNG streams derived from (master seed, hierarchical name); `--seed` also seeds future `gauss()`/MC draws; detector dark-field injection kept as a documented option; name-ordered iteration (fixes B-016). |
| F-05 outputs/tracing | **Approved**: trace only PROBEs/`.save` targets when present, trace-all only as fallback (backward compatible); `--no-trace`; CSV/`.npz` export. |
| F-12 netlist pack | **Approved, SPICE semantics**: `.dc` includes the stop value (one-time golden change); `m`/`M` = milli, `MEG` = mega; legacy ring netlists move to `circuits/legacy/` with a note; SI suffixes, `.ends NAME`, `.dc WL(src)`, `--param`, `.include` relative to the including file. |
| F-28 `.op`/`.dc` convergence | **Approved**: iteration cap, residual report, non-convergence error. |

## CI
- **Approved**: make the sanitizer job blocking once it is clean, using a LeakSanitizer suppression file limited to
  (1) allocations still live at `exit()` on error paths and (2) allocations owned by SystemC thread stacks (never
  unwound by SystemC). Leak detection stays on for everything else.
- **Pass** (stay in backlog): ccache in CI; sanitizer job tuning.

## Execution
New branch `sprint/2026-10b` stacked on `sprint/2026-10` (PR #16); same unattended setup as sprint 2026-10
(paseo workspaces, Opus xhigh orchestrators, golden check per commit, coordinator re-verification before every
merge). Order: harness hardening → approved correctness/physics fixes → F-01 → F-28 → F-12 → F-05, docs alongside.
Push the branch after the final verification (no PR creation, no merge into `dev`).

## Follow-up decisions (2026-10-21, after sprint 2026-10b)
| Item | Decision |
|---|---|
| B-146 | **Land** `P2-ready/b146` as is (JSON `writers` 0 → 1 for nets shared between subcircuit instances; hierarchical = flat). |
| B-014 error | **Option (b)**: error when a non-zero delay of a waveguide written in the netlist rounds to 0 ticks; warning only for delays created inside devices (e.g. the MZI's internal arms, feed-forward). Adapt `P3-ready/b014-error`; `regress/b031-mzi-bidir` keeps passing at the default timescale. |
| Lexer integer overflow | **Clean parse error**, not lexing as doubles (drop the approach of `P4-ready/lexer-int-overflow`). A digit-only literal beyond the `int` range is rejected with its line and the limit (2147483647), instead of being clamped silently. No netlist use needs more: numeric net names, counts (`nstates`), `.options seed` (the full 64-bit range stays available through `--seed`), `maxiter` (`--maxiter` is 64-bit). |
| Zero tolerances | **Allow abstol ≥ 0 and reltol ≥ 0** (netlist `.options`, CLI, config validation of B-018/B-153); negative values stay errors. When both are 0, print one warning: gating is off, feedback circuits never quiesce in `.tran`, `.op`/`.dc` end at `maxiter`. Needed to verify equivalence with the fixed-step discrete model at zero tolerance (it worked in NDEBUG builds before B-018). Add golden cases (open loop at zero tolerance; lossy ring `.tran` at zero tolerance). |
| Next sprint | **F-15 approved** (`discover/scout/F-15-engine-performance-and-accounting.md`), including `--stats` (per run: values scheduled, coalesced, processed, emitted, suppressed; max/mean queue length; last emission time; wall time; JSON next to the trace) and documented expert switches for ablation (no coalescing with FIFO ties; event cap). No new globals. |
| Versioning | **Minor version per upgrade**: v0.2.0 = sprints 2026-10 + 2026-10b + the items above; v0.3.0 = F-15; **v1.0.0 is reserved for the journal publication**. Merges into `dev`/`master`, tags and pushes only on an explicit maintainer request. |
