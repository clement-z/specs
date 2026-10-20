# B-148: User-input validation is done with assert(): abort (134) in Debug, silently accepted in Release
severity: med | category: parser | effort: S | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 3e55eb3
location: src/parser/parse_analysis.cpp:34, src/parser/parse_analysis.cpp:87, src/parser/parse_directive.cpp:21, src/parser/parse_element.cpp:396, :492, :584, :637, :705, :858, :1001, :1084, :1157, :1202, :1275
## Problem
Positional-arg counts and missing analysis arguments are checked with `assert`. In a Debug build, such input
aborts with a core dump (rc 134). In the documented Release build (`-DNDEBUG`), the input is silently accepted:
extra args are ignored, or the run crashes later.
## Evidence
`backlog/repro/a4-asserts.sh <specs>`. Debug = `/tmp/specs-scratch/specs-build/specs` (same sources),
Release = `/tmp/ws-review-build/specs`.

| input | assert | Debug | Release |
|---|---|---|---|
| `.tran 1e-9 2e-9` | parse_analysis.cpp:87 | 134 | rc 0, 2nd arg ignored |
| bare `.dc` | parse_analysis.cpp:34 | 134 | rc 1, `E549 uncaught exception: vector::reserve` |
| `.options 5` | parse_directive.cpp:21 | 134 | rc 0, ignored |
| `pshift1 a b e 1 2 3` | parse_element.cpp:396 | 134 | rc 0 |
| `mzi1 a c b d e 1 2` | :492 | 134 | rc 0 |
| `crossing1 a c b d 1 2 3` | :584 | 134 | rc 0 |
| `cwsrc1 a 7 1.3e-6 0.5 1` | :637 | 134 | rc 0 (args shifted, B-052) |
| `vlsrc1 a "f1" "f2"` | :705 | 134 | rc 1 `Error: Cannot open the file.` (loads "f1") |
| `evlsrc1 e "f1" "f2"` | :858 | 134 | rc 1 same |
| `probe1 a 1 1 1 1 1` | :1001 | 134 | rc 1 `Too many input arguments` (switch default, :1035) |
| `mlprobe1 a [..] [..]` | :1084 | 134 | rc 0 |
| `pwr1 a 1` | :1157 | 134 | rc 0 |
| `pcmcell1 a b 1e-9 10 0.1 0.2` | :1202 | 134 | rc 0 (Tc/Ta set, see B-052) |
| `pdet1 a e 1e-12 1 3` | :1275 | 134 | rc 0 |

The other asserts in the area are internal invariants and should stay: `assert(nets.size() == n_nets)` is
guaranteed by the grammar, plus `parse_tree.cpp:31,58,150,163,183-185,220` and `parse_tree.h:807,867`.
## Expected
A clean `cerr << ...; exit(1);` with the element name, with the same accept/reject rule as the Debug build.
## Suggested fix
Replace each `assert(args.size() <= N);` (and `== 0`) with
```
    if (args.size() > N)
    {
        cerr << name << ": too many positional arguments (" << args.size() << ", expected at most N)" << endl;
        exit(1);
    }
```
- PROBE (:1001): delete the assert; the switch default at :1035 already reports the error.
- `parse_analysis.cpp:34`: `if (sweep_orders.empty()) { cerr << ".dc: missing sweep specification (expected: .dc /element/attribute start stop step)" << endl; exit(1); }`
- `:87`: the same pattern with `.tran:`.
- `parse_directive.cpp:21`: `.options: unexpected positional argument`.

Scope and corrections (orchestrator, after the second check):
- **Skip** `parse_analysis.cpp:34` (bare `.dc`, handled by B-008) and the CWSRC (`:637`), VLSRC (`:705`), EVLSRC (`:858`) and PCMCELL (`:1202`) asserts, which are converted by B-052 together with its index fix. This makes this issue independent of B-052.
- Remaining bounds: PSHIFT 2, MZI 1, CROSSING 2, MLPROBE 1, PWR 0, PDET 2, PROBE (delete the assert; the switch default already reports it), `.tran` and `.options` as given above.
- Keep the `nets.size() == n_nets` asserts: they are grammar-guaranteed.
## Golden impact
None (verified on a scratch build with all conversions).
## Related
B-052, B-052, B-153 (`verifyConfig` asserts in specs.h:138-146 belong to the engine area).

- Discovery seeds: S3.06 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-06.
## Verification
Patched scratch build: every row gives rc 1 with a one-line message. Golden cir/cir-json/opt are unchanged.

**Independent second check (V8, of F-A4-06): ADJUST.** Method: applied the fix literally (bounds from the existing asserts) on scratch copy /tmp/ws-review-V8, rebuilt Release, golden check --atol 0 (44 pass, no notes); ran a4-asserts.sh and a differential -n run of every circuits/*.cir and repro/*.cir (base vs patched)

Problem reproduced: Debug gives 134 on all 14 rows; Release gives the rc values listed. Patched: all 14 rows give rc 1 with one line (PROBE via the existing switch default; `.dc:`, `.options:` and `.tran:` messages as suggested). Golden: no change. Differential run: only 2 netlists change. (1) a1-dc-bare.cir rc 0 -> 1 under -n; this is expected, and decisions.json already says to skip parse_analysis.cpp:34 because B-008 handles it. (2) **a3-pcm-positional.cir (`pcmcell1 in out 1e-12 10 0.85 0.95`) goes from rc 0 to rc 1** with "too many positional arguments (4, expected at most 2)". The PCMCELL code parses 4 positional args (Emelt N Tc Ta), Release accepts them today, and B-052 (which B-052 was merged into) raises the bound to 4. So "keep the Debug accept/reject rule" is wrong for PCMCELL: it would break a netlist that works in Release.
Required change to the suggested fix: PCMCELL (:1202) must use `if (args.size() > 4)` ("expected at most 4"), not 2. All other bounds match the code once B-052 is applied: PSHIFT 2, MZI 1, CROSSING 2, CWSRC 3 (wl, power, phi after the index fix), VLSRC/EVLSRC 1 (file, after `> 0`), MLPROBE 1, PWR 0, PDET 2, PROBE 4 (switch).
Ordering with B-052: land B-052 first or in the same commit. If F-A4-06 lands alone, (a) a 4-arg CWSRC (today the only positional way to set phi) is rejected before the index fix exists, and (b) VLSRC/EVLSRC cannot load a file positionally at all: 1 arg is still ignored and 2 args (which load args[0] in Release today) are rejected. B-052's fix text edits `assert(args.size() <= 2)` -> `<= 4`. If F-A4-06 lands first, that assert no longer exists, so B-052 must change the bound of the new `if` instead.
Golden-impact text "None" is correct. Locations are correct. The `nets.size() == n_nets` asserts are grammar-guaranteed: keeping them is right.

**Orchestrator:** Scope narrowed: the four positional-arg element sites moved to B-052 (V8 found that PCMCELL needs bound 4 and that CWSRC/VLSRC/EVLSRC bounds depend on the index fix).
