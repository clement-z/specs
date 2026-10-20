# B-091: `draw_graph_from_json.py main()` crashes with UnboundLocalError when called without arguments
severity: low | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: utils/draw_graph_from_json.py:125-142 (and the duplicate in pyspecs/, see F-A6-15)
## Problem
Missing `argv[1]` prints a message but continues; `json_file + ".png"` then raises. `main()` also duplicates the body of `draw_graph()` (lines 115-123), which nothing calls.
## Evidence
```
$ python3 -I utils/draw_graph_from_json.py
  File ".../draw_graph_from_json.py", line 133, in main
    out_file = json_file + ".png"
UnboundLocalError: local variable 'json_file' referenced before assignment
```
## Expected
A usage message and exit status 2.
## Suggested fix
```
     try:
         json_file = sys.argv[1]
     except IndexError:
-        print('First and only argument should be a JSON description of the circuit')
+        print('usage: draw_graph_from_json.py circuit.json [output.png]', file=sys.stderr)
+        sys.exit(2)
...
-    with open(json_file) as f:
-        description = json.load(f)
-    dot_src = generate_graph(description)
-    graph = graphviz.Source(dot_src)
-    graph.render(outfile=out_file)
-    print(f'Results written to {out_file}')
+    draw_graph(json_file, out_file)
```
## Golden impact
none.
## Related
B-090, B-092, B-093.

- Discovery seeds: S3.33 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-16.
## Verification
Ran the script without arguments (traceback above); with a JSON from `specs --json` it produced `ad.json.png`.
