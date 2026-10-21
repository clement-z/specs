# P5 progress — F-28 `.op`/`.dc` convergence + F-12 netlist pack (branch `ws/netlist`)

Prompt: `discover/orchestration/2026-10b/P5-netlist.md`. Heartbeat: paseo heartbeat **`50c26c63`** (P5-heartbeat,
every 5 min, expires 48 h). Delete it right after committing `P5-REPORT.md`.

Start: `ws/netlist` @ 0dee6b2 (P1–P4 merged). Builds: gcc `/tmp/P5-gcc`.

## Items
| # | item | owner | status | commit | notes |
|---|---|---|---|---|---|
| 1 | F-28 iteration cap for the `.op`/`.dc` fixed point (`.options maxiter`, `--maxiter`), residual report, non-convergence error + lossless-loop golden case | orchestrator | done | 1be5a5c | repro: lossless all-pass ring k=1e-3/1e-4 off resonance, `.op` > 60 s (k=1e-2: 0.14 s); default 1e6 (golden max 869 deltas, 1000-ring chain 1e5, CROW-10 3.5e4; CROW-20/100 never converge); 9 new cases + cli/help, cli/no-args, gtest |
| 2 | F-12 SI suffixes (`f p n u m k MEG G T`, `m`/`M` milli), also in expressions | Opus implementer `lexgram`, `/tmp/P5-wt/lexgram` (`P5-wip/lexgram`) | done | 6c837e5 | 3 new cases (JSON with exact-rounding checks, `.op`, unit letters rejected) |
| 3 | F-12 `.ends NAME` | `lexgram` | done | e80bba4 | 2 new cases; mismatch error via yyerror with file:line |
| 4 | F-12 `.dc WL(src)` alias of `/src/WL` | `lexgram` | done | b00b8ae | 4 new cases; alias and `/src/WL` outputs identical |
| 5 | F-12 `.dc` includes the stop value (re-record every `.dc` case: one more point) | Opus implementer `semantics`, `/tmp/P5-wt/semantics` (`P5-wip/semantics`) | in progress | | |
| 6 | F-12 `--param name=value` overrides `.param` | `semantics` | in progress | | |
| 7 | F-12 `.include` relative to the including file, then the cwd; warning if both exist and differ | `semantics` | in progress | | |
| 8 | F-12 legacy ring netlists to `circuits/legacy/` + README note; golden case paths | | pending | | |
| 9 | docs (netlist syntax, numerics, README, CHANGELOG, golden README), backlog status, report | | pending | | |

## Decisions (running list, copied into the report)
- F-28 iteration = one delta cycle of the current time step (`runFixedPoint()` steps `sc_start(SC_ZERO_TIME)`); default
  `maxiter` 1000000 per solve (OP, each `.dc` point, the OP before `.tran`); 0 = no limit.
- `--maxiter` is applied directly (like `--seed`, full 64-bit range) and wins over `.options maxiter`; `.options maxiter`
  takes integers 0..2^53.
- Residual = the output ports that emitted in the last tenth of the iterations (at least 100 delta cycles), sorted by
  last change / threshold, 10 listed; message on stderr, exit 1. Nothing is printed for a converging solve (no
  golden change), and `printConfig()`/the VCD config trace are unchanged.
- An idle `.op` still calls `sc_start()` once, to keep SystemC's W571 warning of the old code (golden-neutral).

## Log
- 2026-10-21 — heartbeat 50c26c63 created; context read. Baseline gcc /tmp/P5-gcc: meson test 12/12, golden 153/153 `--atol 0`.
- 2026-10-21 — implementers launched: `lexgram` (items 2-4), `semantics` (items 5-7); orchestrator on item 1 (F-28).
- 2026-10-21 — `lexgram` done (fc55747, 7190c0d, 58ba465 on P5-wip/lexgram), `semantics` done (4abeeb5, 1abd8dd, 5d21f97 on P5-wip/semantics); both diffs reviewed.
- 2026-10-21 — item 1 (F-28) committed 1be5a5c; meson test 12/12, golden 162/162 `--atol 0`.
- 2026-10-21 — items 2-4 cherry-picked (cases.json merged), new cases recorded: 6c837e5, e80bba4, b00b8ae; golden 171/171 `--atol 0`.
