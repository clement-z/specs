"""A6 repro: pyspecs.specs.simulate() argument handling.

usage (from a scratch dir containing circuits/ and traces/):
  python3 -I a6-pyspecs-simulate.py <repo-root> <dir-containing-specs-binary-with-trailing-slash>
Expected on the unfixed tree:
  1. default output file is traces/delete_me.vcd.vcd (SystemC appends .vcd)
  2. the command line carries --abstol 1e-08 --reltol 0.0001 even though add_drop_pyspecs.cir has
     `.options abstol=1e-6 reltol=1e-8` (CLI wins over the netlist)
  3. kwargs abstol=1e-12 plus custom_arguments yields duplicated -o/--abstol/--reltol, the later (default) wins
  4. verbose=True on a successful run prints "SPECS had an error"
  5. simulator_directory without trailing slash -> FileNotFoundError for '<dir>specs'
"""
import os
import sys

sys.path.insert(0, sys.argv[1])
import pyspecs.specs as sp  # noqa: E402

d = sys.argv[2]
net = "circuits/add_drop_pyspecs.cir"
print("--- 1/2: defaults")
sp.simulate(net, simulator_directory=d)
print(sorted(os.listdir("traces")))
print("--- 3/4: kwargs + custom_arguments, verbose=True")
sp.simulate(net, simulator_directory=d, abstol=1e-12, custom_arguments={"n": None, "o": "traces/dup"}, verbose=True)
print("--- 5: no trailing slash")
try:
    sp.simulate(net, simulator_directory=d.rstrip("/"))
except Exception as e:  # noqa: BLE001
    print(type(e).__name__, e)
