# Feature scout (sprint 2026-10, workstream `ws-scout`)

Question answered: *which missing features would add the most value to SPECS's users and to its adoption, at
reasonable cost?* No product code was modified; everything lives in this folder.

| File | Content |
|---|---|
| `FEATURES.md` | Evidence summary, ranked table of 30 features (`F-01`..`F-30`) with value/effort/risk/prerequisites, three-horizon roadmap, dependency graph |
| `F-NN-<slug>.md` | Design briefs for the 15 top-ranked features: problem, prior art, design sketch, touch points (`file:line`), validation plan, effort, open questions |
| `SOURCES.md` | 288 external sources with one-line takeaways, grouped by research strand (A tools/formats, B users, C engineering, D physics) |
| `bench/` | Feasibility scripts and measurements: CROW generator and timing matrix, analytic add-drop oracle, perf profile (`RESULTS.md`) |

Top five by rank: F-01 reproducible runs, F-02 sweeps and Monte-Carlo, F-03 S-parameter device, F-04 receiver
chain, F-05 outputs and tracing control. See `FEATURES.md` §2 for the roadmap.
