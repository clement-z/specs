# B-097: `utils/openwave.sh` bounds check uses the length of the first path; shebang is not portable
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in a77cd9f (ws-cleanup C-06)
location: utils/openwave.sh:34, :1
## Problem
`${#configs}` is the string length of `configs[0]`, not the number of configs. With three configs in `cfg/` (path length 10) entering 9 passes the check and runs `gtkwave -a "" t.vcd`. The shebang `#!/bin/env bash` does not exist on systems without `/bin` -> `/usr/bin` merge (macOS, older Debian); the file is mode 100644 and the Makefile calls `bash utils/openwave.sh`, so it is currently unused, but breaks if someone `chmod +x`.
## Evidence
`backlog/repro/a6-openwave-bounds.sh <repo>`:
```
Your choice ? [0]: Going with  (9).
gtkwave -a "" "t.vcd"
STUB gtkwave args: [-a  t.vcd]
```
## Expected
"Out of bounds" and a re-prompt.
## Suggested fix
```
utils/openwave.sh:1    #!/bin/env bash        ->  #!/usr/bin/env bash
utils/openwave.sh:34   ... -ge ${#configs} ]]; do   ->   ... -ge ${#configs[@]} ]]; do
```
## Golden impact
none.
## Related
Makefile `waves`/`newwaves` (replaced by Meson run_targets, PLAN.md:92: port the logic with the fix).

- Discovery seeds: S3.33 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-22.
## Verification
Applied both edits to a copy in scratch: choice 9 re-prompts, then 2 selects `cfg/b.gtkw`; 0 runs gtkwave without config; -1 re-prompts.

**Independent second check (V5, of F-A6-22): CONFIRMED.** Method: ran the repro against the original; applied both edits to a copy; drove it with a stub gtkwave and several inputs

Original with 3 configs and input 9: "Going with  (9)." then `gtkwave -a "" t.vcd` (stub prints `[-a  t.vcd]`). With `${#configs[@]}` and `#!/usr/bin/env bash`: inputs 9 then 2 and 4 then 3 re-prompt with "Out of bounds" and then select cfg/b.gtkw and cfg/c.gtkw; 3 -> cfg/c.gtkw; 0 -> no config; -1 then 1 -> re-prompt then cfg/a.gtkw. Both edits are single-token. `/bin/env` and `/usr/bin/env` both exist here, so the shebang change cannot be exercised as a failure on this host; it is a portability-only change. Fixer check: `bash -n utils/openwave.sh`, plus the stub-gtkwave script with `printf '9\n2\n'`. With only `9\n` on stdin the fixed script prints "Out of bounds" once, then `read` hits EOF, choice becomes 0 and it falls through to `gtkwave t.vcd` (no hang), so give a valid second answer in the test.
