# B-017: `--json` to an unwritable path reports success and exits 0
severity: low | category: cli | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in 28adebe
location: src/main.cpp:397-405
## Problem
The `ofstream` open failure is ignored. The program prints "Exported flattened circuit as JSON > …" and exits 0.
## Evidence
`specs -n --json /nonexistent/x.json -f …` → `Exported flattened circuit as JSON > /nonexistent/x.json`, rc=0, no file.
## Expected
An error and rc≠0.
## Suggested fix
main.cpp after line 401:
```cpp
            if (!outfile)
            {
                cerr << "Error: cannot open JSON output file \"" << json_filename << "\"" << endl;
                return 1;
            }
```
## Golden impact
None (validated).
## Related
none.

- Review ids: F-A1-17.
## Verification
Ran the repro (`backlog/repro/a1-cli.sh`). With the patch: error message, rc=1, golden unchanged.

**Independent second check (V3, of F-A1-17): CONFIRMED.** Method: ran `specs -n --json /nonexistent/x.json -f ...` before and after the exact patch (inserted after main.cpp:401), golden check --atol 0

Base: "Exported ... > /nonexistent/x.json", rc=0. Patched: `Error: cannot open JSON output file "/nonexistent/x.json"`, rc=1. A writable path still writes the file, rc=0. Golden: 44/44, 0 notes. The same pattern in the dead do_circuit (main.cpp:114-118) goes away with B-026.
