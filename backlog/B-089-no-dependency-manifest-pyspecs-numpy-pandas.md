# B-089: No dependency manifest for pyspecs (numpy, pandas, matplotlib, pyDigitalWaveTools)
severity: low | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: pyspecs/ (missing file), pyspecs/pyspecs_example.ipynb cell 0 comment, pyspecs/draw_graph_from_json.py:4, utils/analyze_detector_trace.py:8
## Problem
The Python side declares its dependencies only in a notebook comment and in the external manual (`pip install numpy matplotlib pandas pyDigitalWaveTools`). `import pyspecs.specs` fails without pandas, matplotlib and pyDigitalWaveTools (specs.py imports them and the wildcard import pulls in specsparse). `draw_graph_from_json.py` needs `graphviz` plus the Graphviz `dot` binary, `analyze_detector_trace.py` needs scipy. `pyspecs` is not installable as a package.
## Evidence
`python3 -c 'import pandas, numpy, pyDigitalWaveTools'` on the system python: `ModuleNotFoundError: No module named 'pandas'`. Everything works in a fresh venv with: numpy 2.0.2, pandas 2.3.3, matplotlib 3.9.4, pyDigitalWaveTools 1.2, graphviz 0.21, scipy 1.13.1 (Python 3.9.21).
## Expected
`pip install -r pyspecs/requirements.txt` gives a working setup.
## Suggested fix
Add `pyspecs/requirements.txt`:
```
numpy
pandas
matplotlib
pyDigitalWaveTools>=1.2
# optional: utils/draw_graph_from_json.py (also needs the Graphviz `dot` binary)
# graphviz
# optional: utils/analyze_detector_trace.py
# scipy
```
A `pyproject.toml` (installable package, `python_requires`) is a follow-up design decision. Cover the pandas 3 incompatibility first (B-077).
## Golden impact
none.
## Related
B-077, B-094, B-084.

- Discovery seeds: S3.16, S5.30 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-14.
## Verification
Created a venv, installed exactly these packages and ran every script/snippet of this review with them.
