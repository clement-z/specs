# P3 progress — devices and physics (branch `ws/devices`)

Prompt: `discover/orchestration/2026-10b/P3-devices.md` (+ `COMMON.md`). Heartbeat: paseo heartbeat `6af166a0`
(every 5 min, expires 48 h; delete after the report is committed).

Base: `ws/devices` @ 51f0ca2 (= `sprint/2026-10b` after the P2 merge). Baseline build `/tmp/P3-build`
(gcc, specs-deps-next): meson test 12/12, golden 115/115 `--atol 0`.

## Items
| # | item(s) | status | commit | notes |
|---|---|---|---|---|
| 1 | B-032 + B-033 + B-159 crossing | in progress | | implementer worktree `/tmp/P3-wt/crossing` (branch `P3-wip/crossing`) |
| 2 | B-049 + B-050 event-driven PDET | in progress | | `/tmp/P3-wt/pdet` (`P3-wip/pdet`) |
| 3 | B-048 probe sampling (IC-proof) | in progress | | `/tmp/P3-wt/probes` (`P3-wip/probes`) |
| 4 | B-021 + B-014 + B-124 numerics | in progress | | `/tmp/P3-wt/numerics` (`P3-wip/numerics`); docs after the code |

## Resume notes
- Implementers commit code/tests/netlists on their `P3-wip/*` branch only; the orchestrator reviews, cherry-picks
  onto `ws/devices` in order 1→4, records the golden cases and does the bookkeeping.
- If an implementer was cut off: check its worktree (`git -C /tmp/P3-wt/<name> status/log`), relaunch with the same
  prompt + "continue from the current state of the worktree".
