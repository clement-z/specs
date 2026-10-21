# B-052: Positional args of CWSRC/VLSRC/EVLSRC are off by one; PCMCELL Tc/Ta is blocked by assert
severity: high | category: parser | effort: XS | fix-class: needs-human | verdict: CONFIRMED | status: fixed in aaa02b3 (sprint 2026-10b P2)
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

Orchestrator additions:
- In the same commit, **replace the four asserts** at `parse_element.cpp:637` (CWSRC), `:705` (VLSRC), `:858` (EVLSRC) and `:1202` (PCMCELL) with the `cerr << ...; exit(1);` pattern of B-148, with bounds CWSRC 3 (wl, power, phi), VLSRC/EVLSRC 1 (file), PCMCELL 4 (Emelt, N, Tc, Ta). B-148 skips these four sites so that the two issues stay independent.
- No positional order is documented anywhere (cheatsheet, paper, KiCad symbols and all `circuits/*.cir` use keywords); the fix keeps the order the code already encodes. Document it in the cheatsheet (B-080) when this lands.
- The "convert from cross-coupling field coef" comments at :647/:660/:663 are also deleted by B-173; whichever lands first.

Classified `needs-human` only because the fix changes the results of netlists that run today (with wrong results). The change itself is mechanical; a maintainer sign-off is enough.
## Golden impact
None. No circuit in `circuits/` uses positional args for these four elements (checked by grep), and the golden cases only use kwargs.
## Related
A4 (asserts used for user-input validation are compiled out in Release: CMAKE_CXX_FLAGS_RELEASE has -DNDEBUG).

- Discovery seeds: S3.02, S2.19, S3.14 (see `backlog/README.md` for seed numbering).
- Merged duplicate reports: F-A4-05 (A4), F-A4-07 (A4).
- Review ids: F-A3-06, F-A4-05, F-A4-07.
## Verification
I ran the three repros with the release binary. The fix is safe because the new indices match what the existing code clearly intends (wl, power, phase order already in the code). Each positional form is currently either garbage or ignored, so no meaningful result changes.

**Independent second check (V1, of F-A3-06): CONFIRMED.** Method: applied the exact fix on the scratch copy, rebuilt, ran the golden check (default and --atol 0: 44/44 pass, so no golden impact), and ran the three repros before and after

- **CWSRC:**
  - Before: `@ 1000000 nm, 0 W`.
  - After: `@ 1550 nm, 0.001000 W`.
- **VLSRC (positional "a3-vl-noheader.csv"):**
  - Before: only `@0 s ... 0 W`.
  - After: emissions at 200 ps (2 mW) and 300 ps (0 W).
  - The 100 ps row is still lost. setValues(filename) always discards the first line, so headerless files lose row 1. That is a separate bug (B-053) and should not be mistaken for a failure of this fix.
- **PCMCELL:** 0.85 mW before and after in Release. The assert only matters with NDEBUG off; the CMake Release flags are `-O3 -DNDEBUG`.
- **Is the order unambiguous:**
  - The cheatsheet (`cwsrc1 1 power=val phi=val wl=val`), the ECOC paper (`cwsrc1 in wl={wl0} power=1`, `vlsrc1 in values="pulse.txt"`), utils/specs.kicad_sym (`wl=1550e-9 power=1`, `values="file.txt"`) and all circuits/*.cir use kwargs only. pyspecs emits none of these elements.
  - So no positional order is documented anywhere. The fix adopts the order the code already encodes (wl, power, phase, matching the kwarg handling order), which is the minimal choice.
  - Every other element indexes positionals from args[0], and parser.y puts nothing in front of them. args[1] is therefore clearly an off-by-one.
  - Recommendation (non-blocking): add the positional order to the cheatsheet when this lands.
- **Behaviour change:** a positional *list* to VLSRC/EVLSRC now exits with "Variable is not a string" instead of being silently ignored. This is acceptable, because it was never valid.
- All cited line numbers are correct.

**Orchestrator:** Reclassified easy-unattended -> needs-human (positional netlists that run today change results); the four asserts of these elements were moved here from B-148 to remove the ordering dependency.
