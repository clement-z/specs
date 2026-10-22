# B-087: `simulate()` prints "SPECS had an error" for every verbose run and validates input with `assert`
severity: low | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in ef0ea37
location: pyspecs/specs.py:110-122, :204
## Problem
`if (not rc == 0) or verbose:` prints "SPECS had an error. Here is the full output:" even when `rc == 0`. `p.wait()` after `communicate()` is redundant and `specs_stderr` is always None (stderr is merged into stdout). `assert((netlist_file is not None) or (custom_arguments is not None))` is the user-input validation CLAUDE.md says not to do with assert (it vanishes under `python -O`).
## Evidence
`simulate(..., verbose=True)` on a successful run (rc 0): output starts with `SPECS had an error. Here is the full output:` (a6-pyspecs-simulate.py section 3/4).
## Expected
The error banner only when rc != 0, a neutral header for verbose output, `ValueError` for missing input.
## Suggested fix
```
-    specs_stdout, specs_stderr = p.communicate()
-    p.wait()
+    specs_stdout, _ = p.communicate()
...
-    if (not specs_return_code == 0) or (verbose):
-        print("SPECS had an error. Here is the full output: ")
+    if specs_return_code != 0:
+        print(f"SPECS failed with code {specs_return_code}. Here is the full output: ")
+    elif verbose:
+        print("SPECS output: ")
+    if specs_return_code != 0 or verbose:
         for line in specs_stdout.splitlines():
             print(line.decode(encoding='utf-8', errors='ignore'))
...
-    assert((netlist_file is not None) or (custom_arguments is not None))
+    if netlist_file is None and custom_arguments is None:
+        raise ValueError("simulate() needs netlist_file or custom_arguments")
```
## Golden impact
none.
## Related
B-079.

- Discovery seeds: S3.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-12.
## Verification
Read lines 100-124 and ran the verbose call; the banner appears with return code 0.

**Independent second check (V5, of F-A6-12): CONFIRMED.** Method: applied the exact diff on a scratch copy; ran success, verbose, failing-netlist and no-argument cases; ran the original under `python -O`

Original: with `verbose=True` and rc 0 the header is "SPECS had an error..." (reproduced by reading lines 119-120 and by the diff). Fixed: the non-verbose success prints nothing extra; verbose success prints "SPECS output:" and the output; a failing run (missing netlist, rc 1) prints "SPECS failed with code 1. Here is the full output:". The return tuple types are unchanged (float, int, bytes). With no arguments it raises `ValueError: simulate() needs netlist_file or custom_arguments`. The original under `python -O` silently skips the assert and tries to run `specs` with no arguments. Nothing in the repo catches AssertionError or matches on the old banner. The message strings are the reviewer's wording, a trivial choice that is fully specified in the finding.
