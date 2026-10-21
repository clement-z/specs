# Phase A — decided follow-ups for v0.2.0 (branch `ws/followups`)

Follow `discover/orchestration/2026-10c/COMMON.md` (heartbeat first). Phase id: `PA`. Opus 5.5 xhigh; Opus
implementer subagents allowed for items 2–4; review every diff. Decisions: `backlog/DECISIONS-2026-10-21.md`,
section "Follow-up decisions". Read the P2, P3 and P4 reports in `backlog/sprint-2026-10b/` for context.

| Order | Item | How | Expected golden impact (anything else = stop and report) |
|---|---|---|---|
| 1 | **B-146** | Cherry-pick `8129020` (`P2-ready/b146`) unchanged in code; resolve the `cases.json`/`manifest.json` conflicts as a union; re-record only what it predicts | `cir-json/subckt_tests` `flat.json`: `writers` of `ROOT/out` and `ROOT/X2/N00001` 0 → 1; plus its 5 new cases |
| 2 | **B-014 error, option (b)** | Start from `P3-ready/b014-error` (cfe0eb0) but do **not** take its timescale change to `b031`: error (clean message, exit 1, before the simulation) only when a waveguide written in the netlist has a non-zero delay that rounds to 0 ticks; delays of waveguides created inside devices (e.g. MZI arms) keep the warning. Make the distinction explicit in code (where the waveguide comes from), not by name matching | none on existing cases; `regress/b031-mzi-bidir` unchanged; new: netlist waveguide rounding to 0 → error case; device-internal rounding → warning-only case |
| 3 | **Lexer integer overflow → parse error** | Not the double-lexing of `P4-ready/lexer-int-overflow`. A digit-only literal beyond the `int` range is a parse error with file:line and the limit (2147483647), wherever integers are lexed (kwargs, positional args, net names, `.options`). Remove P4's `.options seed` guard against the clamped value 2147483647 (no longer needed) | `err/f01-options-seed-huge-integer`: new message; `err/f01-options-seed-int-max` becomes a passing case (rename it `regress/f01-options-seed-int-max`, VCD identical to `--seed 2147483647`); new error cases for a kwarg, a positional argument and a net name beyond the range |
| 4 | **Zero tolerances allowed** | `abstol ≥ 0` and `reltol ≥ 0` in `.options`, CLI (`--abstol`, `--reltol`) and config validation (B-018/B-153 checks); negative values stay errors. When both are 0, print one warning (stderr) saying gating is off, feedback circuits never quiesce in `.tran`, and `.op`/`.dc` end at `maxiter`. Update `doc/numerics.md` | `err/b153-reltol-zero` becomes a passing case (rename to `regress/`), stderr only if abstol is also 0; new: open-loop circuit at zero tolerance (exact, hand-checked); lossy ring `.tran` at zero tolerance (terminates by decay); both zero → exactly one warning; `.op` on a lossless loop at zero tolerance stops at `maxiter` with the F-28 error |

Docs: `doc/numerics.md` (zero tolerances, delay rounding error rule), `doc/netlist.md` (integer limits),
CHANGELOG `[unreleased]`, backlog status of B-146, B-014, B-018, B-153 and the lexer item (add a backlog entry
`B-179` for the integer clamp if none exists).
