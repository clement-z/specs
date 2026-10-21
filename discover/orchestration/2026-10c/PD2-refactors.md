# Phase D2 — bit-identical engine refactors (branch `ws/engine2`) — ready once the F-15 answers are confirmed

Follow `COMMON.md` and `PD-common-f15.md`. Phase id: `PD2`. One commit per refactor, each gated on bit-identical
outputs and on the `--stats` counters of D1 [Q1]:
1. tolerance gate on squared norms (unit tests for `|last| = 0`, NaN, infinities);
2. per-wavelength S-parameter caches (waveguide, coupler, phase shifter): same arithmetic, same bits;
3. pending events: keep today's order among equal-time entries [Q8] (largest |E| first, including the effect of
   in-place merges), so keep the heap and its comparator; replace `sc_event_queue` by a single `sc_event` and remove
   per-event allocations; speed up the same-time lookup only in ways that keep the pop order identical;
4. `OpticalSignal`: drop the virtual destructor; the per-copy id goes behind a compile-time flag, off by default
   [Q2], in its own commit (the only output change of D2: listings lose the `[NNNN]` prefix, values unchanged,
   re-recorded with that note).
Measure after each step with `discover/scout/bench/` (ratios only; shared host) and report.
