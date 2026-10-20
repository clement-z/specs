# B-148: User-input validation is done with assert(): abort (134) in Debug, silently accepted in Release
severity: med | category: parser | effort: S | fix-class: easy-unattended | verdict: CONFIRMED
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

Skip `parse_analysis.cpp:34` (bare `.dc`): it is handled by B-008.
## Golden impact
None (verified on a scratch build with all conversions).
## Related
B-052, B-052, B-153 (`verifyConfig` asserts in specs.h:138-146 belong to the engine area).

- Discovery seeds: S3.06 (see `backlog/README.md` for seed numbering).
- Review ids: F-A4-06.
## Verification
Patched scratch build: every row gives rc 1 with a one-line message. Golden cir/cir-json/opt are unchanged.
