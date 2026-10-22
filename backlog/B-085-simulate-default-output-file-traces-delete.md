# B-085: `simulate()` default output file is `traces/delete_me.vcd.vcd`
severity: low | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in a921bc0
location: pyspecs/specs.py:19-20, :150-153 (docstring)
## Problem
`-o` takes a basename: SystemC appends `.vcd`. The default `"o": "traces/delete_me.vcd"` therefore produces `traces/delete_me.vcd.vcd`, and the docstring documents the wrong default. The notebook passes `../traces/delete_me` (no extension), so it is not affected.
## Evidence
```
Command: specs -f circuits/add_drop_pyspecs.cir -o traces/delete_me.vcd --abstol 1e-08 --reltol 0.0001
os.listdir('traces') -> ['delete_me.vcd.vcd']
```
## Expected
`traces/delete_me.vcd`.
## Suggested fix
```
pyspecs/specs.py:19   "o": "traces/delete_me.vcd",   ->   "o": "traces/delete_me",
pyspecs/specs.py:150-153 docstring: "output_file ... Base name of the VCD file; SPECS appends '.vcd'. Defaults to "traces/delete_me" (-> traces/delete_me.vcd)."
```
(Not stripping a user-supplied `.vcd` suffix: that would be an API addition.)
## Golden impact
none.
## Related
B-079 (same dict), B-007.

- Discovery seeds: S3.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-10.
## Verification
`backlog/repro/a6-pyspecs-simulate.py` section 1 (file list shows `delete_me.vcd.vcd`).

**Independent second check (V5, of F-A6-10): CONFIRMED.** Method: ran simulate() with the prebuilt binary before/after the one-line default change

Before: `-o traces/delete_me.vcd` produces `traces/delete_me.vcd.vcd`. After changing line 19 to `"traces/delete_me"`: the file is `traces/delete_me.vcd`. The golden case opt/tracefile-o (`-o traces/custom` -> custom.vcd) confirms that `-o` takes a basename. The default dict is also used on the `custom_arguments` path, which gets the same fix. Docstring location is lines 150-153; the dict entry is line 19 only (line 20 is `abstol`; the stated range "19-20" is harmless). Same file and dict as B-079, B-086, B-087 and B-094, so apply those sequentially with content-anchored edits. Fixer check: `python3 -m py_compile`, then simulate(netlist_file=..., simulator_directory="/tmp/ws-review-build/") in a cwd with traces/ and check the file name.
