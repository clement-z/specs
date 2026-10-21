# F-15 — shared notes for phases D1–D3 (DRAFT, not launchable until "F-15 decisions" exist)

Brief: `discover/scout/F-15-engine-performance-and-accounting.md`. Decision: `backlog/DECISIONS-2026-10-21.md`,
"Follow-up decisions" (F-15 approved: bit-identical refactors, `SC_METHOD` conversion, single timed queue,
`--stats`, documented expert ablation switches, no new globals). The maintainer's answers to the open questions
go into a new section "F-15 decisions"; each phase prompt below cites them as **[Q1]…[Q8]**.

Acceptance (all D phases): golden `check --atol 0` passes with no re-record except the items that predict one;
for D2 and D3 additionally the outputs are compared bit for bit (`--rtol 0 --atol 0 --print-slack 0`) and the
`--stats` counters per case are compared as [Q1] says.
