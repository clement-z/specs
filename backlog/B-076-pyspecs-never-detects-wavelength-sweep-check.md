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
