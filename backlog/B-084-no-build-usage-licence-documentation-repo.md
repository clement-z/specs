# B-084: No build/usage/licence documentation in the repo; mainpage introduction is "TODO"; README is a pointer to an external site
severity: med | category: docs | effort: M | fix-class: needs-design | verdict: CONFIRMED | status: partly: README has Building, running, testing, pixi, citation (sprint 2026-10b/c); no licence line, no Python install section, `doc/mainpage.md` Introduction still TODO, stray emoji selector in Copyright remains
location: README.md:1-17, doc/mainpage.md:3-5, doc/papers/, doc/resources/
## Problem
`README.md` is one paragraph linking to `demo.zrounba.fr/specs/doc` (reachable, but its source is not versioned), a contributing hint and the citation. There is no build, run, test or Python section, no licence line, no list of dependencies; the only in-repo build knowledge is in `CLAUDE.md`/`discover/`. `doc/mainpage.md` (the Doxygen start page) has an empty "Introduction: TODO". The "Copyright" line contains a stray U+FE0F emoji variation selector (`©` + EF B8 8F) which makes some renderers show an emoji.
## Evidence
`cat -A README.md`: `Copyright M-BM-)M-oM-8M-^O Institut ...`; mainpage.md line 5 `TODO`; the online manual sections "Getting started", "Using the Python interface" and "Reference" have no in-repo counterpart.
## Expected
README with: what SPECS is, build/run/test, Python wrapper install, link to reference, licence; mainpage that introduces the project.
## Suggested fix
Write README sections (Building, Running, Testing, Python, Documentation, Licence) and an Introduction on the mainpage; consider importing the external manual source. Obsoleted-by-meson: partly. PLAN.md step 5 adds the README "Building" section; the rest (Running, Python, Licence, Introduction) is independent. Immediate micro-fix that needs no design: delete the U+FE0F byte sequence in README.md line 17 (`©️` becomes `©`).
## Golden impact
none.
## Related
B-080, B-083, B-089, B-105, B-104.

- Discovery seeds: S5.09 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-09.
## Verification
Read README/mainpage, fetched the external index (HTTP 200) and compared the section list.
