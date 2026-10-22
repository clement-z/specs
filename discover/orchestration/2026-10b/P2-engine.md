# P2 — netlist and engine correctness (branch `ws/engine`)

Follow `discover/orchestration/2026-10b/COMMON.md` (heartbeat first). Phase id: `P2`. You are an Opus 5.5 xhigh
orchestrator: use Opus implementer subagents for parser/engine items, Sonnet for docs, and review every diff.
P1 (golden harness hardening) is merged: `check` is strict by default now; read its report
(`backlog/sprint-2026-10b/P1-REPORT.md`).

Items, as decided in `backlog/DECISIONS-2026-10-21.md` (read each backlog file, its repro files and second-check notes):
| Order | Item(s) | Expected golden impact (anything else = stop and report) |
|---|---|---|
| 1 | B-001 FD queue merge (validated patch) | none on existing cases; add `backlog/repro/a1-fd-multilambda-stale.cir` as a golden case (expect 0.5625 W at 1550 nm) |
| 2 | B-144 + B-145 + B-147 bidir slot convention (start from `backlog/repro/v7-bidir-slots-prototype.diff`, extend to all devices) | only circuits where a uni writer precedes the bidir device, or crossings on bidir nets; add every `a4-*`/`v7-*` bidir repro as golden cases with hand-checked expected values |
| 3 | B-146 subckt port nets via `nets_flat` | none on existing cases; add the `v7-x-shared-*` repros (hierarchical == flat) |
| 4 | B-052 positional args (+ replace the 4 asserts with clean errors) | none on existing cases (examples use keywords); new positional and too-many-args cases |
| 5 | B-051 PCMCELL requires `tc`/`ta`; align constructor defaults with the header; `OctaneCell` gets the values as **required constructor parameters** (no silent physics default) | none unless a case builds a PCMCELL without tc/ta (then it becomes a clean error, predicted) |
| 6 | B-060 validation only (N > 0, 0 ≤ k ≤ N) | new error cases only |
| 7 | B-002 + B-004: reject `sampled-time` at configuration time; explicit `-m` overrides the netlist directive's mode | `opt/mode-sampled-time` becomes a clean error; `opt/mode-fd` (and any case passing `-m` with an analysis directive) runs in the requested mode: re-record, explain the diff |
| 8 | B-058 remove `TimeMonitor` and `--mon` | `cli/help` and `cli/no-args` lose the `--mon` line |
| 9 | B-037 docs: `k` = field coupling (P_cross = k²), `kp` = power, `t` = through field; the paper's κ is a different parameter (power), no erratum; remove the per-instance warning, fix the comments | stdout of cases with couplers loses the warning line |
Ordering constraints from `backlog/SUMMARY.md` apply (B-147 never without B-144; B-052 owns the four asserts B-148 skipped).
