# Phase D2 — bit-identical engine refactors (branch `ws/engine2`) — ready once the F-15 answers are confirmed

Follow `COMMON.md` and `PD-common-f15.md`. Phase id: `PD2`. One commit per refactor, each gated on bit-identical
outputs and on the `--stats` counters of D1 [Q1]:
1. tolerance gate on squared norms (unit tests for `|last| = 0`, NaN, infinities);
2. per-wavelength S-parameter caches (waveguide, coupler, phase shifter): same arithmetic, same bits;
3. pending events [Q8]: order by **time only**; drop the \|E\| comparison (`OpticalSignal::operator<` inversion) and use
   a deterministic insertion order among equal times; replace `sc_event_queue` by a single `sc_event` and remove
   per-event allocations; same-time (t, λ) merge as a lookup. Do this in its own commit: it is the one D2 change
   allowed to alter outputs, and only for plain probes/VCD on multi-wavelength nets (which wavelength is the last
   write of a time step); re-record exactly those cases, show that every other case is bit-identical and that
   per-wavelength values (MLPROBE, tables, device memories) are unchanged;
4. `OpticalSignal`: drop the virtual destructor; the per-copy id goes behind a compile-time flag, off by default
   [Q2], in its own commit (the only output change of D2: listings lose the `[NNNN]` prefix, values unchanged,
   re-recorded with that note).
Measure after each step with `discover/scout/bench/` (ratios only; shared host) and report.
