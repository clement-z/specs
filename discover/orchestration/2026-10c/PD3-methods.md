# Phase D3 — `SC_METHOD` conversion (branch `ws/methods`) — DRAFT

Follow `COMMON.md` and `PD-common-f15.md`. Phase id: `PD3`. Convert `OpticalOutputPort::on_data_ready` and the
stateless device reactions (waveguide, coupler, splitter, merger, crossing, phase shifter) to `SC_METHOD`; keep
threads where `wait()` with time is needed (detector tick, sources). Naming per [Q3]. Outputs must stay bit-identical;
event counters per [Q1] (evaluation order inside a delta cycle may change). Update `CLAUDE.md` (process style).
Final: per-event cost before/after on the CROW benchmark (ratios), target ≥ 1.5× on the bench's case C (N = 30).
