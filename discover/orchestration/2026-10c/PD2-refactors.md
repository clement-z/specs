# Phase D2 — bit-identical engine refactors (branch `ws/engine2`) — DRAFT

Follow `COMMON.md` and `PD-common-f15.md`. Phase id: `PD2`. One commit per refactor, each gated on bit-identical
outputs and on the `--stats` counters of D1 [Q1]:
1. tolerance gate on squared norms (unit tests for `|last| = 0`, NaN, infinities);
2. per-wavelength S-parameter caches (waveguide, coupler, phase shifter): same arithmetic, same bits;
3. one ordered pending structure per output port (`(time, λ)` keys; same-time merge = lookup; earliest = begin)
   and a single `sc_event` replacing `sc_event_queue`; equal-time ordering per [Q8];
4. `OpticalSignal`: virtual destructor and per-copy id per [Q2].
Measure after each step with `discover/scout/bench/` (ratios only; shared host) and report.
