# B-086: `simulator_directory` needs a trailing slash (string concatenation instead of a path join)
severity: low | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 30f6820
location: pyspecs/specs.py:231-232
## Problem
`[simulator_directory + simulator_name]` turns `"../build"` into `"../buildspecs"`.
## Evidence
```
simulate(net, simulator_directory="/tmp/ws-review-build")
-> FileNotFoundError [Errno 2] No such file or directory: '/tmp/ws-review-buildspecs'
```
## Expected
Works with and without trailing slash; `""` still means "look in PATH".
## Suggested fix
```
pyspecs/specs.py:  add `import os` (top); line 232:
-    simulator_command = [simulator_directory + simulator_name]
+    simulator_command = [os.path.join(simulator_directory, simulator_name)]
```
`os.path.join("", "specs") == "specs"` and `os.path.join("./", "specs") == "./specs"`, so existing callers are unchanged.
## Golden impact
none.
## Related
B-088 (notebook uses `../build/`).

- Discovery seeds: S3.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-11.
## Verification
`backlog/repro/a6-pyspecs-simulate.py` section 5; semantics of `os.path.join` for `""`, `"./"`, `"/abs/"`, `"/abs"`.

**Independent second check (V5, of F-A6-11): CONFIRMED.** Method: applied `import os` + `os.path.join` on a scratch copy and called simulate() with 5 different simulator_directory values

With the fix: "/tmp/ws-review-build", "/tmp/ws-review-build/", "/tmp/ws-review-build//", "./" (binary in the cwd) and "" (specs found via PATH) all run with rc 0. The original code fails without a trailing slash, since `dir + "specs"` becomes `.../buildspecs`. The import line (line 4, between numpy/pandas and the subprocess import) collides with the B-094 import rewrite. Apply B-094 first or add `import os` next to `import subprocess`.
