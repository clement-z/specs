# B-094: Unused imports, debug prints and dead code in pyspecs; wildcard import re-exports third-party names
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
location: pyspecs/specs.py:1-3,6,8-16,90,102,108; pyspecs/specsparse.py:4-5,24,129-137
## Problem
`pyflakes`: specs.py imports numpy, pandas and matplotlib without using them (forcing matplotlib for a call that only runs a subprocess and slowing `import pyspecs.specs`); specsparse.py imports `matplotlib.pyplot` and `json` unused, assigns `simulation_data_df` (line 24) never used and contains an `if False:` block (lines 129-137) plus commented prints. specs.py has a pseudo-docstring "Ideas" string after the imports, commented-out `spec_env`/`Popen` lines (102, 108) and a reference to a non-existent `par_run()` (line 90). `from .specsparse import *` (no `__all__`) exports `np`, `pd`, `plt`, `json`, `VcdParser` and the internals as `specs.*`.
## Evidence
```
pyspecs/specsparse.py:4:1: 'matplotlib.pyplot as plt' imported but unused
pyspecs/specsparse.py:5:1: 'json' imported but unused
pyspecs/specsparse.py:24:5: local variable 'simulation_data_df' is assigned to but never used
pyspecs/specs.py:1:1: 'numpy as np' imported but unused   (:2 plt, :3 pd)
```
## Expected
Only used imports; an explicit public API.
## Suggested fix
Delete the unused imports and the dead lines listed above; replace the wildcard with `from .specsparse import parse_vcd_dataframe, verify_wl_sweep, find_by_time` (the three public functions the notebook uses as `specs.parse_vcd_dataframe`, `specs.verify_wl_sweep`, `specs.find_by_time`); turn the "Ideas" string into a `#` comment. Do not remove the `print('--- Contents of simulation ---')` summary (user-visible).
## Golden impact
none.
## Related
B-076 (removes the `print(child)` debug lines), B-077.

- Discovery seeds: S3.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-19.
## Verification
`pyflakes pyspecs/*.py utils/*.py` output as above; grep of the notebook for `specs.<name>` uses.
