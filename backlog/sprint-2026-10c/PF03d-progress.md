# PF03d progress: SPAR decisions of the second discussion (branch `ws/spar4`)

Prompt: `discover/orchestration/2026-10c/PF03d-spar-final.md`. Decisions: `backlog/DECISIONS-2026-10-21.md`, row
"SPAR second discussion"; details and expected values: `SPAR-discussion-2.md`. Base: fd239c8.

Heartbeat: `44fdc915` (PF03d-heartbeat, every 5 min, expires in 48 h).

Before builds: worktree `/tmp/PF03d-wt/base` (fd239c8, branch `PF03d-wip/base`): gcc, clang, asan-gcc.
Prototype (Opus subagent): `/tmp/PF03d-proto` (outside the repository, not durable).

## Items

| # | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | Remove `delay_per` | in progress | | |
| 2 | Grid check on the actual intervals | todo | | |
| 3 | PCHIP slopes per run | todo | | |
| 4 | Pulse-pair tau0 | todo | | |
| 5 | Docs and report | todo | | |

## Algorithms (written before the code, shared with the prototype)

Item 2, `leaveOneOutErrors()`: point j is interpolated in re/im parts when
`realImaginaryInterval(t, j - 1, j) || realImaginaryInterval(t, j, j + 1)` (was `(t, j - 1, j + 1)`).

Item 3, PCHIP magnitude/phase slopes per run: interval k = [k, k+1] is re/im when
`realImaginaryInterval(t, k, k + 1)` (after `removeDelay()`). A run is a maximal sequence of consecutive
magnitude/phase intervals; `dmag`/`dphase` at the points of a run = `pchipSlopes()` of the run's points alone (two
points: the secant); points of no run keep 0 (never used). `dre`/`dim` unchanged (whole grid). Leave-one-out:
the window j-2..j+2 without j; its intervals are [j-2, j-1] (re/im as on the grid), [j-1, j+1] (re/im by item 2's
rule), [j+1, j+2] (as on the grid); for a magnitude/phase point the slopes come from the run of the window that
contains [j-1, j+1]. Without re/im intervals: bit-identical to before.

Item 4, `removeDelay()` tau0 (m >= 2): steps h_k = f[k+1] - f[k]; median step (median of h, the mean of the two
middle values for an even count); interval k is used when h_k <= 3 * median step. z_k = S(k+1) conj S(k) (file
values re/im), Z = sum of z_k over used k in ascending k, mean step = (sum of used h_k) / count. tau0 =
arg(Z) / (2 pi mean step) (0 if Z == 0). Then two Newton steps on
g(tau) = sum_k |z_k| sin(arg z_k - 2 pi h_k tau) h_k, g'(tau) = -2 pi sum_k |z_k| cos(arg z_k - 2 pi h_k tau) h_k^2
(used k), tau -= g / g', a step taken only when g' < 0 and the result is finite. Then the 1e-3 rad cut over the
largest step of the whole grid and f_ref = first grid point, unchanged.

## Predictions

(written before each re-record)

## Log
