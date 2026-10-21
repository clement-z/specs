# P2 progress — netlist and engine correctness (branch `ws/engine`)

Prompt: `discover/orchestration/2026-10b/P2-engine.md` (+ `COMMON.md`). Heartbeat: `7a89fad1` (`P2-heartbeat`,
every 5 min, expires 48 h after 2026-10-21 01:5x UTC). Delete it after the report is committed.

Build dir: `/tmp/P2-build` (gcc, `-Dcmake_prefix_path=$HOME/.local/opt/specs-deps-next`). Helper scripts (not in
the repo): `/tmp/P2-tools/addcase.py` (append a golden case), `/tmp/P2-tools/setstatus.py` (issue + backlog.md status).
Start state (d214196): `meson test` 12/12, golden `--atol 0` 81/81.

| # | item(s) | status | commit | notes |
|---|---|---|---|---|
| 1 | B-001 FD queue merge | done | b2bd836 | validated patch; gtest + golden `regress/b001-fd-multilambda-stale`; existing cases unchanged |
| 2 | B-144 + B-145 + B-147 bidir slots | pending | | |
| 3 | B-146 subckt port nets via `nets_flat` | pending | | |
| 4 | B-052 positional args + 4 asserts | pending | | |
| 5 | B-051 PCMCELL requires tc/ta | pending | | |
| 6 | B-060 PCM validation | pending | | |
| 7 | B-002 + B-004 sampled-time / `-m` | pending | | |
| 8 | B-058 remove TimeMonitor / `--mon` | pending | | |
| 9 | B-037 coupler k docs | pending | | |

## Decisions taken
- (none yet)

## Found, not done
- (none yet)
