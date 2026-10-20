# B-052: Positional args of CWSRC/VLSRC/EVLSRC are off by one; PCMCELL Tc/Ta is blocked by assert
severity: high | category: parser | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: src/parser/parse_element.cpp:637-652,714-715,867-868,1202,1215-1218
## Problem
- **CWSRC**: `args[1]` → WL, `args[2]` → power, `args[3]` → phase. `args[0]` is ignored, and `args[3]` is behind `assert(args.size() <= 3)`. `cwsrc1 in 1.55e-6 1e-3` therefore emits 0 W at λ = 1 mm.
- **VLSRC / EVLSRC**: `if(args.size() > 1) obj->setValues(args[0]...)` is behind `assert(args.size() <= 1)`. A positional values file is silently ignored, so the source emits only its initial 0.
- **PCMCELL**: the code handles 4 positional args (Emelt N Tc Ta) but asserts `args.size() <= 2`. Debug builds abort; Release (NDEBUG) accepts them. Behaviour therefore depends on the build type.

The comments "convert from cross-coupling field coef" at :647, :660 and :663 are copy-paste leftovers from DCElement.
## Evidence
- `backlog/repro/a3-cwsrc-positional.cir`: `ROOT/CWSRC1 emitted: ... @ 1000000 nm (0.000000 V.m⁻¹, 0.000000 W ...)`.
- `backlog/repro/a3-vlsrc-positional.cir` (run in the repro dir): VLSRC1 emits only `@0 s ... 0 W`. The 0.1/0.2/0.3 ns rows of `a3-vl-noheader.csv` are never used, and there is no error.
- `backlog/repro/a3-pcm-positional.cir` works in Release (out = 0.85 mW), but the assert at :1202 fires in a Debug build.

Additional evidence (independent report F-A4-05, A4 reviewer):

`backlog/repro/a4-cwsrc-positional.cir` (`cwsrc1 a 1.3e-6 0.5`): rc 0,
`ROOT/CWSRC1 emitted: ... @ 5e+08 nm (0.000000 V.m⁻¹, 0.000000 W ...)`, i.e. wl = 0.5 m and P = 0.
`cwsrc1 a 7 1.3e-6 0.5` gives 1300 nm and 0.5 W (the 7 is dropped).

Additional evidence (independent report F-A4-07, A4 reviewer):

Code at :714 `if(args.size() > 1) obj->setValues(args[0].as_string());` (same at :867). In Release,
`vlsrc1 a "f1" "f2"` gives `Error: Cannot open the file.` (it opened "f1"). `pcmcell1 a b 1e-9 10 0.1 0.2` gives
rc 0 in Release and 134 in Debug.
## Expected
Positional order is CW `wl power phi`, VL/EVL `file`, PCM `emelt n tc ta`.
## Suggested fix
```
src/parser/parse_element.cpp:646-652
-    if(args.size() > 1)
-        // convert from cross-coupling field coef
-        obj->setWavelength(args[1].as_double());
-    if(args.size() > 2)
-        obj->setPower(args[2].as_double());
-    if(args.size() > 3)
-        obj->setPhase(args[3].as_double());
+    if(args.size() > 0)
+        obj->setWavelength(args[0].as_double());
+    if(args.size() > 1)
+        obj->setPower(args[1].as_double());
+    if(args.size() > 2)
+        obj->setPhase(args[2].as_double());
(also delete the two "// convert from cross-coupling field coef" comments at :660, :663)
:714  -    if(args.size() > 1)      +    if(args.size() > 0)        (VLSRC)
:867  -    if(args.size() > 1)      +    if(args.size() > 0)        (EVLSRC)
:1202 -    assert(args.size() <= 2); +    assert(args.size() <= 4);  (PCMCELL)
```
## Golden impact
None. No circuit in `circuits/` uses positional args for these four elements (checked by grep), and the golden cases only use kwargs.
## Related
A4 (asserts used for user-input validation are compiled out in Release: CMAKE_CXX_FLAGS_RELEASE has -DNDEBUG).

- Discovery seeds: S3.02, S2.19, S3.14 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A4-05 (A4), F-A4-07 (A4).
- Review ids: F-A3-06, F-A4-05, F-A4-07.
## Verification
I ran the three repros with the release binary. The fix is safe because the new indices match what the existing code clearly intends (wl, power, phase order already in the code). Each positional form is currently either garbage or ignored, so no meaningful result changes.
