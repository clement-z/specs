# B-094: Unused imports, debug prints and dead code in pyspecs; wildcard import re-exports third-party names
severity: low | category: style/maintainability | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 5a8b0f2 (ws-cleanup C-06)
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

Corrections from the second check (apply them):
- The `par_run()` reference is in the docstring at `specs.py:84-85`, not :90: delete only "For this case, use par_run()." and keep the sentence ending at "usage.".
- Also delete the commented-out prints in `specsparse.py` (lines 23, 54-56).
- **Do not** replace the wildcard import in this issue: explicit names narrow the public API of `pyspecs.specs` (`check_wl_sweep`, `is_probe`, `vcd_get_*`, `VcdParser`, `np`, `pd`, `plt`, `json` would disappear) and need `__all__`; that is a maintainer decision.
- Apply this issue **last** among the pyspecs issues (B-076/02/03/10/11/12 edit the same files), or use content-anchored edits.
- Fixer check: `python3 -m py_compile pyspecs/*.py`, `pyflakes pyspecs/specs.py pyspecs/specsparse.py`, and parse the 27 baseline VCDs before/after (identical DataFrames).
## Golden impact
none.
## Related
B-076 (removes the `print(child)` debug lines), B-077.

- Discovery seeds: S3.16 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-19.
## Verification
`pyflakes pyspecs/*.py utils/*.py` output as above; grep of the notebook for `specs.<name>` uses.

**Independent second check (V5, of F-A6-19): ADJUST.** Method: pyflakes before/after; applied the listed removals on a scratch copy; parsed all 27 baseline VCDs and ran the notebook cells and simulate()

The problem is real and the change is behaviour-neutral: pyflakes reports exactly the quoted items; after the edits all 27 parse results are `equals` to the originals; simulate() works; the notebook cells 0-9 run (it only uses specs.simulate / parse_vcd_dataframe / verify_wl_sweep / find_by_time and its own np/pd/plt imports). Adjustments needed: (1) location: the `par_run()` reference is at specs.py:84-85 (docstring "This is BLOCKING execution ... For this case, use par_run()."), not :90 (line 90 is "its directory and simulator name. Ex: ./specs"). Fix text: delete "For this case, use par_run()." and keep the sentence ending at "usage."; (2) the "commented prints" in specsparse.py (lines 23, 54-56) are not in the listed line set, so the finding is silent on them. Either state them explicitly or declare them out of scope; (3) after replacing the wildcard with explicit names, pyflakes reports the three names as "imported but unused" in specs.py. Add `__all__ = [...]` (or accept the pyflakes noise) as part of the fix; (4) narrowing the wildcard removes `specs.<internal helper>` names (create/run helpers stay; `check_wl_sweep`, `is_probe`, `vcd_get_*`, `VcdParser`, `np`, `pd`, `plt`, `json` go away). Nothing in the repo uses them, but this is a tiny public-API narrowing the maintainer should know about; (5) this finding overlaps in line ranges with B-076/02/03/10/11/12 (same two files; deleting lines 129-137 and 24 shifts later line numbers), so it must be applied last or with content-anchored edits. Fixer check: `python3 -m py_compile pyspecs/*.py`, `pyflakes pyspecs/specs.py pyspecs/specsparse.py`, and the 27-VCD parse comparison (identical to before).

**Orchestrator:** Suggested fix amended with the second check's corrections; the wildcard-import change was removed from the easy fix (public-API narrowing).
