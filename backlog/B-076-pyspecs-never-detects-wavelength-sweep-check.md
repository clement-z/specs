# B-076: pyspecs never detects a wavelength sweep: `check_wl_sweep` compares to 'b01' but SPECS writes 'b1'
severity: high | category: python | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: pyspecs/specsparse.py:258-266 (also :262,:264 debug prints)
## Problem
`check_wl_sweep` tests `analysis_type == 'b01'`. SystemC writes the 32-bit `analysis_type` vector without leading zeros and pyDigitalWaveTools returns it verbatim, so CW_SWEEP (=1, `src/specs.h:75-80`) is `'b1'` and TIME_DOMAIN (=2) is `'b10'`. `'b01'` matches nothing, so every `.dc` VCD is parsed as time-domain: no `wavelength` column, the x axis `time` is `sweep_index * 1 fs`, and `verify_wl_sweep(df)` returns False (the notebook then plots a sweep against `time`). The whole FD branch of `parse_vcd_dataframe` is dead code. It also prints two debug lines (`print(child)`, `print(f'analysis_type:...')`) on every call.
## Evidence
```
tests/golden/baseline/cases/cir/add_drop/files/traces/delete_me.vcd:11   $var wire   32  aaabt  analysis_type [31:0]  $end
                                                                :127  b1 aaabt          (circuits/add_drop.cir uses `.dc`)
pyDigitalWaveTools 1.2 -> {'name': 'analysis_type', 'type': {'width': 32, 'name': 'wire'}, 'data': [(0, 'b1')]}
$ python3 -I backlog/repro/a6-pyspecs-parse-vcds.py <repo> .../cir/add_drop/files/traces/delete_me.vcd
... probes=(30, 31) ... ['Type of simulation: TD']          <- wrong, it is a .dc sweep
```
Same misclassification for the other three `.dc` baselines (opt/mode-fd, opt/multi-file, opt/tol-resolution-overrides); all TD VCDs carry `b10`. With `== 'b1'` the same file prints `Type of simulation: FD`, first column `wavelength` = 1.535e-06 ... 1.564e-06.
## Expected
`.dc` VCDs are recognised as FD: first column `wavelength`, no `time` column.
## Suggested fix
```
pyspecs/specsparse.py:258-266
     for child in top_scope['children']:
         if child['name'] == 'analysis_type':
-            print(child)
-            analysis_type = child['data'][0][1]
-            print(f'analysis_type:{analysis_type}')
-            return analysis_type == 'b01'
+            # SystemC strips leading zeros from the vector: 'b1' = CW_SWEEP, 'b10' = TIME_DOMAIN (AnalysisType, src/specs.h)
+            analysis_type = child['data'][0][1]
+            return int(analysis_type[1:], 2) == 1
```
Do this together with B-077 and B-078 in one commit (overlapping lines): `backlog/repro/a6-specsparse-fix.diff` already contains all three.
## Golden impact
none (no golden case runs pyspecs).
## Related
B-078 (fixing only this exposes the fragile wavelength merge in sweeps whose first probe is an unpowered net), B-096 (no test would have caught it). discover/03 §6.

- Discovery seeds: S3.15 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-01.
## Verification
Ran pyspecs from the repo against all 27 baseline VCDs in a venv (python 3.9, pandas 2.3.3, numpy 2.0.2, pyDigitalWaveTools 1.2): the 4 `.dc` files report TD, the 23 time-domain ones report TD. With the fix the 4 report FD and the 23 TD dataframes are bit-identical to before (`DataFrame.equals`). Seed S3.15 was right on this point; the pyDigitalWaveTools open question in discover/03 §7 is answered (it does not normalise `b1` to `b01`).

**Independent second check (V5, of F-A6-01): CONFIRMED.** Method: read pyDigitalWaveTools 1.2 parser source + all 27 baseline VCDs; ran pyspecs on pandas 2.3.3 (py3.9) and pandas 3.0.6 (py3.12) before/after the fix

Cause proven from three sources. (1) SPECS: `sc_trace(tf, (int&)s.analysis_type, ...)` (src/specs.cpp:388); the VCDs contain `b1` for the 4 `.dc` cases (cir/add_drop, opt/mode-fd, opt/multi-file, opt/tol-resolution-overrides) and `b10` for the other 23. No `b01` occurs anywhere. (2) pyDigitalWaveTools 1.2 `vcd_value_change` stores the token verbatim (`value = token`), with no normalisation. (3) Running the original code: all 27 files print "Type of simulation: TD", including the four `.dc` files, so the first column is `time` and there is no `wavelength` column. With the finding's fix applied alone (checked on pandas 2.3.3 and pandas 3.0.6 with fix 02 also applied): the 4 `.dc` files give FD (first column `wavelength`, 1.535e-06 to 1.564e-06, shape (30,31), 0 NaN, no `time` column). The 23 TD DataFrames are `DataFrame.equals` identical to the original, for both df_probes and df_pdet. The suggested fix is unique and local. `int(x[1:], 2)` is safe for what SystemC writes (b0/b1/b10). A 'bx' value would raise, but SystemC does not emit that for an int trace. Severity `high` is defensible: all `.dc` results are misparsed on the wrapper's only read path.
Fixer check: `python3 -m py_compile pyspecs/specsparse.py`. Then, in a venv with pandas + numpy + matplotlib + pyDigitalWaveTools, run `parse_vcd_dataframe` on the 27 `tests/golden/baseline/cases/*/*/files/**/*.vcd`. Expect FD for exactly the 4 listed files and unchanged TD frames for the other 23. Line numbers shift if B-094 or B-078 are applied first, so use content-anchored edits.
