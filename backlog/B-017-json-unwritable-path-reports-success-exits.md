# B-017: `--json` to an unwritable path reports success and exits 0
severity: low | category: cli | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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
