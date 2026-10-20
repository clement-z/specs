# B-079: `simulate()` always passes --abstol/--reltol, silently overriding the netlist `.options`, and duplicates flags when both kwargs and custom_arguments are given
severity: med | category: python | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: pyspecs/specs.py:18-22, :26-43, :204-229
## Problem
`default_settings` (abstol 1e-8, reltol 1e-4, o) is applied unconditionally. Since commit 1b0780d command-line values win over `.options` in the netlist, so a pyspecs run never honours the netlist tolerances: the shipped example `circuits/add_drop_pyspecs.cir` has `.options abstol=1e-6 reltol=1e-8 timescale=-12` and the notebook (which uses it) simulates with 1e-8/1e-4, giving a different event sequence than `specs -f circuits/add_drop_pyspecs.cir`. When `netlist_file` and `custom_arguments` are both passed, `-o/--abstol/--reltol` are emitted twice and the later one (the defaults) silently wins over the `abstol=` kwarg.
## Evidence
```
simulate("circuits/add_drop_pyspecs.cir") -> Command: specs -f circuits/add_drop_pyspecs.cir -o traces/delete_me.vcd --abstol 1e-08 --reltol 0.0001
VCD of that run:  r1e-08 (abstol)  r0.0001 (reltol)  979 time steps
direct `specs -f circuits/add_drop_pyspecs.cir -o traces/direct`:  r1e-06 / r1e-08, 1250 time steps
simulate(net, abstol=1e-12, custom_arguments={"n": None, "o": "traces/dup"}) ->
  specs -f net -o traces/delete_me.vcd --abstol 1e-12 --reltol 0.0001 -o traces/dup --abstol 1e-08 --reltol 0.0001 -n
  (trace file used: traces/dup; the abstol=1e-12 argument is lost)
```
`backlog/repro/a6-pyspecs-simulate.py`.
## Expected
Without explicit arguments the netlist (or SPECS) defaults apply; explicit kwargs / custom_arguments are each emitted once, custom_arguments overriding kwargs.
## Suggested fix
Build one ordered dict: `f` (if any), `o` (output_file, default as in B-085), then `abstol`/`reltol` only if the caller supplied them (kwargs), then `update(custom_arguments)`; pass it through `create_arguments_dictionary` once. Remove `abstol`/`reltol` from `default_settings`. Update the docstring ("Defaults to 1e-8" becomes "defaults to the netlist `.options` / SPECS default 1e-8") and drop the explicit tolerances from notebook cells 2-3 (they are then the same as the CLI run). Design call: the documented default of the public API changes.
## Golden impact
none.
## Related
B-085, B-086, B-088 (notebook).

- Review ids: F-A6-04.
## Verification
Ran `simulate()` against the prebuilt binary (command line printed above), diffed the two VCDs (abstol/reltol values and `#` timestamps: 979 vs 1250).
