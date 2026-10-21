# PA progress (branch `ws/followups`)

Heartbeat: `2852972a` (PA-heartbeat, every 5 min, expires in 48h). Delete after committing PA-REPORT.md.

| Order | Item | Status | Commit | Notes |
|---|---|---|---|---|
| 1 | B-146 | done | 5912b6a | union of cases.json/manifest.json; 5 new cases re-recorded with current build, `unordered_streams` dropped; 195/195 `--atol 0`; gcc + ASan meson test 12/12 |
| 2 | B-014 error (b) | done | a23e4f9 | `m_from_netlist` set by `WGElement::create()`; regress→err rename of the rounds-to-zero case + 2 new cases; b031 unchanged; 197/197 `--atol 0`; gcc + ASan meson test 12/12 |
| 3 | 64-bit integer literals (B-180) | done | bd2465c | Opus implementer (0e1fede), reviewed; 10 golden cases (int-max renamed err→regress); VCD identities checked; 205/205 `--atol 0` gcc and ASan (with lsan.supp); meson test 12/12 both |
| 4 | Zero tolerances | done | 26327cb | Opus implementer (a2cc395), reviewed; 9 golden cases (reltol-zero renamed err→regress) + `err/b153-abstol-negative` message `> 0`→`>= 0` (not in the table, reported); 212/212 `--atol 0` gcc and ASan (with lsan.supp); meson test 12/12 both |
| — | Docs / report | in progress (final verification, report) | | |

## Notes
- ff4d2e2: merged `sprint/2026-10c` (8af20cc, B-181 `Variable` initialisers) at the coordinator's request, before item 3.
- Builds: `/tmp/PA-gcc` (gcc release), `/tmp/PA-asan` (clang ASan+UBSan debug).
- 2026-10-21, maintainer instruction: new subagents get a model chosen per task (haiku/sonnet/opus) and an explicit effort of medium or high (supersedes "Opus implementer subagents" in the phase prompt). The two implementers already running (items 3, 4: Opus, default effort) were not restarted.
