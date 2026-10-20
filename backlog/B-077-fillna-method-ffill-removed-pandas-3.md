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
