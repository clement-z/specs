# B-090: `draw_graph_from_json.py` exists twice, byte for byte
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 5a8b0f2 (ws-cleanup C-06)
location: pyspecs/draw_graph_from_json.py, utils/draw_graph_from_json.py
## Problem
Two identical copies (same blob `78a3c64`), referenced from nowhere (not by the Makefile, docs, the `pyspecs` package or the notebook). Fixes (B-091..18) would have to be applied twice.
## Evidence
`cmp pyspecs/draw_graph_from_json.py utils/draw_graph_from_json.py` is silent; `git ls-files -s` shows the same blob; `grep -rn draw_graph_from_json` finds only discover/ text.
## Expected
One copy.
## Suggested fix
`git rm pyspecs/draw_graph_from_json.py` (keep `utils/`: it is a CLI helper, not part of the `pyspecs` import API).
## Golden impact
none.
## Related
B-091, B-092, B-093.

- Discovery seeds: S5.22, S3.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-15.
## Verification
`cmp` and `git ls-files -s`; nothing imports the module.

**Independent second check (V5, of F-A6-15): CONFIRMED.** Method: cmp, git ls-files -s, repo-wide grep

Both files have blob 78a3c64 (byte-identical); nothing outside discover/ and backlog/ mentions the module (Makefile, install.sh, notebook, README do not). Caveat: keeping `utils/` and removing the `pyspecs/` copy is a small judgement call, because it removes the importable name `pyspecs.draw_graph_from_json`. The reviewer's rationale (CLI helper, not API) is reasonable, and B-089 also names the utils/ path. If the maintainer prefers the opposite, the fix is mirrored, but nothing in the repo depends on either. Acceptable for unattended execution. Fixer check: `git rm pyspecs/draw_graph_from_json.py`; `grep -rn draw_graph_from_json --exclude-dir=.git --exclude-dir=discover --exclude-dir=backlog .` must not list pyspecs/.
