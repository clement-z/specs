# B-093: `draw_graph_from_json.py` fails on Graphviz builds without the triangulation library and leaves a `.gv` file next to the output
severity: low | category: python | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: utils/draw_graph_from_json.py:101-102, :141
## Problem
The generated DOT uses `layout="sfdp"` with `overlap=false`, which needs Graphviz built with GTS. On this machine `dot` prints `Error: remove_overlap: Graphviz not built with triangulation library`, exits 1 (PNG is still written) and graphviz-py raises `CalledProcessError`, so the script dies with a traceback. `Source.render(outfile=...)` also keeps the intermediate `<out>.gv` in the output directory.
## Evidence
```
$ python3 -I utils/draw_graph_from_json.py ad.json
Error: remove_overlap: Graphviz not built with triangulation library
graphviz.backend.execute.CalledProcessError: Command '[... 'dot', '-Kdot', '-Tpng', '-o', 'ad.json.png', 'ad.json.gv']' returned non-zero exit status 1.
ls: ad.json  ad.json.gv  ad.json.png
```
## Expected
Renders on stock Graphviz, no stray intermediate.
## Suggested fix
Choose: `overlap=scale`/`prism`-free alternatives (`overlap=false` -> `overlap=scale`), or catch `CalledProcessError` and keep the PNG; pass `cleanup=True` to `render`. Layout quality is a taste decision.
## Golden impact
none.
## Related
B-091.

- Review ids: F-A6-18.
## Verification
Ran the tool on the JSON from `specs -f circuits/add_drop_pyspecs.cir --json ad.json -n` (distro Graphviz without GTS).
