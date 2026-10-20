# B-090: `draw_graph_from_json.py` exists twice, byte for byte
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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
