# B-097: `utils/openwave.sh` bounds check uses the length of the first path; shebang is not portable
severity: low | category: style/maintainability | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
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
