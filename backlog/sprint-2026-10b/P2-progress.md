# P2 progress — netlist and engine correctness (branch `ws/engine`)

Prompt: `discover/orchestration/2026-10b/P2-engine.md` (+ `COMMON.md`). Heartbeat: `7a89fad1` (`P2-heartbeat`,
every 5 min, expires 48 h after 2026-10-21 01:5x UTC). Delete it after the report is committed.

Build dir: `/tmp/P2-build` (gcc, `-Dcmake_prefix_path=$HOME/.local/opt/specs-deps-next`). Helper scripts (not in
the repo): `/tmp/P2-tools/addcase.py` (append a golden case), `/tmp/P2-tools/setstatus.py` (issue + backlog.md status).
Start state (d214196): `meson test` 12/12, golden `--atol 0` 81/81.

| # | item(s) | status | commit | notes |
|---|---|---|---|---|
| 1 | B-001 FD queue merge | done | b2bd836 | validated patch; gtest + golden `regress/b001-fd-multilambda-stale`; existing cases unchanged |
| 2 | B-144 + B-145 + B-147 bidir slots | implementing (Opus, wt bidir) | | |
| 3 | B-146 subckt port nets via `nets_flat` | implementing (Opus, wt bidir, after item 2) | | |
| 4 | B-052 positional args + 4 asserts | done | aaa02b3 | 4 positional + 4 too-many-args golden cases; `doc/netlist.md` created; existing cases unchanged |
| 5 | B-051 PCMCELL requires tc/ta | done | 344d60b | presence check incl. aliases; PCMElement default Ta = 1; OctaneCell/Segment/Matrix take Tc/Ta as required params (meltEnergy/nStates defaults of OctaneCell/OctaneMatrix dropped: C++ forbids required after defaulted); 3 golden cases |
| 6 | B-060 PCM validation | done | c22ff40 | `PCMElement::check_parameters()` (parser + `start_of_simulation`); n = 0 keeps its old message; 2 gtests + 5 golden cases; model untouched |
| 7 | B-002 + B-004 sampled-time / `-m` | implementing (Opus, wt cli) | | |
| 8 | B-058 remove TimeMonitor / `--mon` | implementing (Opus, wt cli) | | |
| 9 | B-037 coupler k docs | implementing (Sonnet, wt docs) | | |

## Implementer worktrees (all branched from 7a99a64)
| wt | path / branch | items | build dir | proposed golden cases |
|---|---|---|---|---|
| bidir | `/tmp/P2-wt/bidir` / `P2-wip/bidir` | 2, 3 | `/tmp/P2-bidir-build` | `/tmp/P2-wt/bidir-*.cases.json` |
| pcm | `/tmp/P2-wt/pcm` / `P2-wip/pcm` | 4, 5, 6 | `/tmp/P2-pcm-build` | `/tmp/P2-wt/pcm-*.cases.json` |
| cli | `/tmp/P2-wt/cli` / `P2-wip/cli` | 7, 8 | `/tmp/P2-cli-build` | `/tmp/P2-wt/cli-*.cases.json` |
| docs | `/tmp/P2-wt/docs` / `P2-wip/docs` | 9 | `/tmp/P2-docs-build` | none (re-record only) |

Implementers commit code, tests and new netlists on their branch; they do not touch cases.json, the baseline or
the bookkeeping. Integration (orchestrator, in item order on `ws/engine`): review the diff, cherry-pick, add the
cases (`addcase.py`), build, `meson test`, golden check, record only the predicted cases, status + progress commit.
On resume: if a wt branch has its commits but the agent is gone, integrate from the branch; if it has partial
uncommitted work, relaunch an implementer on that worktree with the same brief (see the item rows above).
Shared new doc file: `doc/netlist.md` (pcm writes CWSRC/VLSRC/EVLSRC/PCMCELL sections, docs writes COUPLER):
merge both at integration.

## Decisions taken
- Integration order 1, 4..9, then 2, 3: the bidir implementer (items 2-3) took longer and items 4-9 do not depend
  on it (constraints B-147 with B-144 and B-146 after B-144 are kept). Each item still has its own commit and
  its own golden re-record, so every baseline change stays attributable.

## Found, not done
- (none yet)
