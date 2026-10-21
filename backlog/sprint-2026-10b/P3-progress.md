# P3 progress — devices and physics (branch `ws/devices`)

Prompt: `discover/orchestration/2026-10b/P3-devices.md` (+ `COMMON.md`). Heartbeat: paseo heartbeat `6af166a0`
(every 5 min, expires 48 h; delete after the report is committed).

Base: `ws/devices` @ 51f0ca2 (= `sprint/2026-10b` after the P2 merge). Baseline build `/tmp/P3-build`
(gcc, specs-deps-next): meson test 12/12, golden 115/115 `--atol 0`.

## Items
| # | item(s) | status | commit | notes |
|---|---|---|---|---|
| 1 | B-032 + B-033 + B-159 crossing | **done** | 04c7819 | from `P3-wip/crossing` 496818c; golden: 4 `b147-crossing-out-*` re-recorded (a/b/d slot split only), 5 new cases, gtest; 120/120 `--atol 0`; docs `doc/netlist.md` CROSSING |
| 2 | B-049 + B-050 event-driven PDET | **done** | be96f26 | from `P3-wip/pdet` 81ccd4c + tb self-check threshold 1e-7 → 1e-12 (0.3 % flake); golden: tb/det, tb/detector re-recorded (3/3, noisy readouts in vcd_ignore_values), 3 new cases, gtest; 123/123; docs `doc/netlist.md` PDET |
| 3 | B-048 probe sampling (IC-proof) | implemented, to integrate after item 2 | | `P3-wip/probes` 8521e6b (implementer done; 10 regress cases change, checker OK) |
| 4 | B-021 + B-014 + B-124 numerics | in progress | | `/tmp/P3-wt/numerics` (`P3-wip/numerics`); docs after the code |

## Resume notes
- Implementer agent ids (Agent tool, SendMessage to resume): crossing a7a716e5b802628f9, pdet afe95174b3cb6aab9,
  probes a5d3360dd44737675, numerics a784da990e3dcb2ac (all Opus, launched 2026-10-21).
- Implementers commit code/tests/netlists on their `P3-wip/*` branch only; the orchestrator reviews, cherry-picks
  onto `ws/devices` in order 1→4, records the golden cases and does the bookkeeping.
- If an implementer was cut off: check its worktree (`git -C /tmp/P3-wt/<name> status/log`), relaunch with the same
  prompt + "continue from the current state of the worktree".
