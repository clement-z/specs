# P5 — F-28 frequency-domain convergence + F-12 netlist pack (branch `ws/netlist`)

Follow `discover/orchestration/2026-10b/COMMON.md` (heartbeat first). Phase id: `P5`. Opus 5.5 xhigh orchestrator;
Opus for grammar/engine work, Sonnet for docs and examples. Briefs: `discover/scout/F-28*` (in `FEATURES.md`) and
`discover/scout/F-12-netlist-compatibility-pack.md`.

F-28 (decided): iteration cap for the `.op`/`.dc` delta-cycle fixed point (`.options` key + CLI, sensible default),
residual report, clean non-convergence error (non-zero exit) instead of hanging. Expected golden impact: none for
converging cases; add a lossless-loop case that must now fail cleanly.

F-12 (decided, SPICE semantics):
- SI suffixes (`f p n u m k MEG G T`, case-insensitive; `m`/`M` = milli, `MEG` = mega), also inside expressions.
- `.ends NAME`, `.dc WL(src) start stop step` alias, `--param name=value` overrides of `.param`.
- `.dc` includes the stop value (one-time golden change for every `.dc` case: one more point; re-record and say so).
- `.include` resolves relative to the including file first, then the working directory (backward compatible: the
  examples include `circuits/...` relative to the cwd); warn if both exist and differ.
- Move the three legacy ring netlists (`double_ring`, `triple_ring`, `paul_simple_ring`) to `circuits/legacy/` with a
  README note (pre-release syntax, kept for reference); update their golden case paths (outputs must not change).
Update the cheatsheet sources / docs with the full syntax, and add golden cases for every new construct.
