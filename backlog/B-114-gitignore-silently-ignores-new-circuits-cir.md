# B-114: `.gitignore` silently ignores new circuits/*.cir and the unanchored `specs` hits any directory named specs
severity: low | category: build | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED
location: .gitignore:25,26,31
## Problem
`*.cir` (line 31) ignores every netlist repo-wide; all 9 files under `circuits/` and `traces/delete_me.vcd` are tracked only because they were force-added (`git ls-files -ci --exclude-standard` lists exactly those 10). A contributor adding `circuits/new.cir` gets no `git status` entry, so the example is silently uncommitted (CLAUDE.md documents `git add -f` as the workaround). Also `specs` and `libspecs.so` (lines 25-26) are unanchored: a package directory such as `pyspecs/specs/` or `src/specs/` would be ignored too. Survives the migration (the Meson port only adds build-dir entries).
## Evidence
```
$ git check-ignore -v circuits/new_example.cir pyspecs/specs/x.py src/specs/a.cpp
.gitignore:31:*.cir	circuits/new_example.cir
.gitignore:25:specs	pyspecs/specs/x.py
.gitignore:25:specs	src/specs/a.cpp
```
(`backlog/repro/a7-gitignore-cir.sh`)
## Expected
`circuits/*.cir` visible to git; only the build products `specs` and `libspecs.so` at the repo root ignored.
## Suggested fix
```
.gitignore:25  specs        -> /specs
.gitignore:26  libspecs.so  -> /libspecs.so
.gitignore:31  *.cir        (keep)  + append:  !/circuits/*.cir
```
`tests/golden/.gitignore` already re-includes `*.cir` below its directory, so it needs no change.
## Golden impact
none.
## Related
B-007, B-116 (Meson build-dir entries).

- Discovery seeds: S3.31 (see `backlog/README.md` for seed numbering).
- Review ids: F-A7-10.
## Verification
Applied the three edits to a copy of `.gitignore` in a scratch git repo: `circuits/new.cir` and `pyspecs/specs/x.py` are no longer ignored, a root `specs` file and `top.cir` still are. The `!` negation works because `circuits/` itself is not excluded.
