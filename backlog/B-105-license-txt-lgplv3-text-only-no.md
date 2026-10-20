# B-105: LICENSE.txt is the LGPLv3 text only: no GPLv3 text, no copyright holder statement, no per-file notice
severity: low | category: docs | effort: S | fix-class: needs-human | verdict: CONFIRMED
location: LICENSE.txt:1-166, README.md:17
## Problem
LGPLv3 incorporates GPLv3 by reference (its first paragraphs), so the GPLv3 text (`COPYING`) should ship with it. There is no "or later" choice, the only copyright line is in the README ("Copyright (c) Institut des Nanotechnologies de Lyon (INL)") while the external manual credits R. Cardoso and C. Zrounba, no SPDX tags or file headers, and SystemC (Apache-2.0 with NOTICE) is linked in; binary distributions (releases, `libspx.so`, the demo site) need its LICENSE/NOTICE.
## Evidence
`head -12 LICENSE.txt` shows "GNU LESSER GENERAL PUBLIC LICENSE Version 3"; `grep -c "GNU General Public License" LICENSE.txt` -> 0; `grep -rIl "SPDX\|Copyright" src` -> none.
## Expected
A licence set the maintainers chose deliberately and that is complete.
## Suggested fix
Maintainer decision: LGPL-3.0-only vs -or-later, copyright holders, then add `COPYING` (GPL-3.0 text), a `LICENSES/Apache-2.0` + SystemC NOTICE pointer for binary releases, and SPDX headers (or a REUSE.toml). Not mechanical because it is a legal choice.
## Golden impact
none.
## Related
B-084.

- Discovery seeds: S5.26 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-30.
## Verification
Read the file and README; discover/05 §8.
