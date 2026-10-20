# B-077: `fillna(method='ffill')` is removed in pandas 3: every time-domain parse raises TypeError
severity: high | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: pyspecs/specsparse.py:104, pyspecs/specsparse.py:146
## Problem
Both forward-fill calls use the `method=` keyword. It is deprecated since pandas 2.1 (FutureWarning on every parse today) and removed in pandas 3.0, which a fresh `pip install pandas` now installs on Python >= 3.11 (3.0.6 is current). `parse_vcd_dataframe` then fails for every time-domain trace (line 104) and every trace with photodetectors (line 146).
## Evidence
```
pandas 3.0.6 wheel, pandas/core/generic.py:6922:   def fillna(self, value, *, axis=None, inplace=False, limit=None)   <- no `method`
pandas 2.3.3 run of the 23 TD baselines: FutureWarning: DataFrame.fillna with 'method' is deprecated and will raise in a future version. Use obj.ffill() or obj.bfill() instead.
```
(pandas 3 itself could not be executed: the machine has Python 3.9 only; the wheel source was inspected.)
## Expected
No warning on pandas 2.x, no crash on pandas 3.
## Suggested fix
```
pyspecs/specsparse.py:104   df_probes = df_probes.fillna(method='ffill')   ->   df_probes = df_probes.ffill()
pyspecs/specsparse.py:146   df_pdet = df_pdet.fillna(method='ffill')       ->   df_pdet = df_pdet.ffill()
```
## Golden impact
none.
## Related
B-076, B-078 (same lines), B-089 (declare supported pandas).

- Discovery seeds: S3.15 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-02.
## Verification
Replaced both calls in a scratch copy and compared `(df_probes, df_pdet)` with the original code for all 23 time-domain baseline VCDs (including tb/detector and tb/det which have pdet data): `DataFrame.equals` True for every file. `ffill()` is the documented equivalent of `fillna(method='ffill')`.

**Independent second check (V5, of F-A6-02): CONFIRMED.** Method: executed the original and fixed code on pandas 3.0.6 (py3.12, installed via uv) and pandas 2.3.3 (py3.9) for all 27 baseline VCDs

Original code on pandas 3.0.6: all 27 files raise `TypeError: NDFrame.fillna() got an unexpected keyword argument 'method'`. This includes the 4 `.dc` files, which go down the TD path because of B-076. On pandas 2.3.3 each parse emits a FutureWarning "DataFrame.fillna with 'method' is deprecated". With `.ffill()` at the two sites: 0 warnings on 2.3.3, and DataFrames `equals` the original on 2.3.3 for all 27 (df_probes and df_pdet; the pdet frames are non-empty only for tb/det and tb/detector). The pandas 3.0.6 results with the fix (together with fix 01) are `equals` to the pandas 2.3.3 results. pandas 3.0.6 requires Python >=3.11 (METADATA), and `fillna(self, value, *, axis, inplace, limit)` has no `method` parameter. Only these 2 uses of `fillna(method=)` exist in pyspecs/ and utils/. Severity `high` is acceptable. By the reviewer brief's own definition (crash on a common path) every time-domain parse crashes on a default `pip install pandas` on Python >=3.11. `med` would also be defensible since pandas 2.x keeps working, so no change required. Suggested fix and (empty) golden impact are correct.
Fixer check: `python3 -m py_compile`, then the same 27-file parse comparison. If possible also run on pandas 3 (`uv python install 3.12` plus `pip install pandas` works offline-ish here). Line 146 shifts to about 137 if B-094 is applied first, so anchor on content.
