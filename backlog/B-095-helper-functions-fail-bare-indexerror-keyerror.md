# B-095: Helper functions fail with bare IndexError/KeyError on edge inputs
severity: low | category: python | effort: S | fix-class: needs-design | verdict: CONFIRMED
location: pyspecs/specsparse.py:165-183 (`verify_wl_sweep`), :185-222 (`find_by_time`)
## Problem
`verify_wl_sweep(df_pdet)` raises `IndexError` for the empty frame that `parse_vcd_dataframe` returns when a circuit has no photodetector; `find_by_time(df, col, t)` raises `IndexError` for `t` before the first sample and `KeyError: 'time'` on a wavelength-sweep frame; its exact `==` float compare on time is fragile; the docstring promises "closest previous value".
## Evidence
```
verify_wl_sweep(pd.DataFrame())       -> IndexError: list index out of range
find_by_time(df_tran, col, -1e-12)    -> IndexError: index 0 is out of bounds for axis 0 with size 0
find_by_time(df_sweep, col, 1.0)      -> KeyError: 'time'
```
## Expected
`False` / documented exception.
## Suggested fix
`verify_wl_sweep`: `return len(df.columns) > 0 and df.columns[0] == 'wavelength'`. `find_by_time`: use `df['time'].searchsorted(time, side='right') - 1` (O(log n), no float equality) and raise `ValueError` with a message for t before the first sample / non-time frames. Exception types are an API decision.
## Golden impact
none.
## Related
B-096.

- Review ids: F-A6-20.
## Verification
Ran the three calls on parsed baseline VCDs.
