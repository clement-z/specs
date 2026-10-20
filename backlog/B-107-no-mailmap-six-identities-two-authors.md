# B-107: No `.mailmap`: six identities for two authors in `git shortlog`
severity: low | category: docs | effort: XS | fix-class: needs-design | verdict: CONFIRMED
location: (missing) .mailmap
## Problem
`git shortlog -sne` lists Clement Zrounba four times (cern.ch, ec-lyon.fr, a github noreply address, "Clement Viken Zrounba") and Raphael Cardoso twice (`rcardoso`, `cardosorapha`). Authorship and contributor statistics are fragmented. The other parts of seed S5.29 (local `dev` ahead of `origin/dev`, `master` behind) are repository state, not file defects.
## Evidence
```
11 Clément Zrounba <clement.zrounba@cern.ch>   8 ...@ec-lyon.fr   5 6691770+clement-z@users.noreply.github.com   4 Clement Viken Zrounba   4 rcardoso   3 cardosorapha
```
## Expected
One canonical identity per person.
## Suggested fix
Add `.mailmap` with canonical names/emails. Which address to publish is the authors' decision.
## Golden impact
none.
## Related
B-105.

- Discovery seeds: S5.29 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-32.
## Verification
`git shortlog -sne HEAD`.
