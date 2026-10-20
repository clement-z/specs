# ws-scout — missing-feature scout

You are the **feature scout** for the SPECS sprint 2026-10 (Fable 5.1, xhigh effort). You run in your own paseo
worktree on branch `ws/scout`, branched from `sprint/2026-10`. A coordinator merges your branch locally.
**Never `git push`. Do not modify product code.** Write only under `discover/scout/`.

## Context to absorb first
- `CLAUDE.md`, `discover/ONBOARDING.md`, all `discover/0*.md` (report 05 §10 has "Seeds for feature scout").
- The paper: `discover/paper-zrounba2023-specs-ecoc.txt` (claims, future work, benchmarks).
- `todo.txt`, `README.md`, `doc/`, `circuits/`, `pyspecs/`, `utils/` (incl. the KiCad symbol library).
- Who uses it: an INL Lyon research group (photonic neural networks / OCTANE PCM cells, PUFs, CROW/ring filters,
  WDM). The online demo is at demo.zrounba.fr/specs.

## Question
**Which missing features would add the most value to SPECS's users (photonic-circuit researchers) and to its
adoption, at reasonable cost?** Think broadly: physics and device models, analyses, the engine
(performance, parallelism, mixed-mode), the input language and tool integration (KiCad, gdsfactory, Touchstone /
S-parameter import, SAX/Simphony/Photontorch/Lumerical INTERCONNECT interop), the Python API and packaging,
outputs (formats, plotting, post-processing), reproducibility (seeds, benchmarks vs paper), developer experience
(tests, docs, examples, CI), and distribution (pip wheels, conda, containers, WebAssembly for the demo).

## Method
- Survey comparable tools and the current state of the art. Web research is allowed: cite sources. Note what
  users of those tools take for granted that SPECS lacks, and what SPECS's event-driven approach uniquely enables
  (e.g. large-scale time-domain PNN simulation, PCM dynamics, noise/Monte-Carlo).
- Ground every proposal in the code: say where it would plug in (file:line), what existing pieces it reuses, and
  what blocks it (link known defects from `discover/` reports; a backlog is being written in parallel in another
  workspace, so don't duplicate bug-hunting).
- You may spawn subagents (Agent tool) for parallel research strands. Keep the total under about 6.
- You may build and run SPECS to validate feasibility claims. Build:
  `cmake -S . -B /tmp/ws-scout-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=$HOME/.local/opt/specs-deps && ninja -C /tmp/ws-scout-build specs`.

## Deliverables (`discover/scout/`)
- `FEATURES.md`: ranked table: `F-NN | feature | user value (1–5) | effort (S/M/L/XL) | risk | prerequisites |
  rank rationale`, then a recommended roadmap in 3 horizons (quick wins under 1 week; next quarter; strategic).
- `F-NN-<slug>.md` for the top ~12: problem/user story, prior art (with links), proposed design sketch, touch points
  in the code (file:line), validation plan (how to test it, ideally against the paper or analytic results), effort
  breakdown, open questions for the maintainer.
- `SOURCES.md`: every external source consulted, with a one-line takeaway each.
Commit on `ws/scout` (`scout: …`), then stop and
print a 15-line summary with the top 5 features.
