# B-083: Doxyfile lists the deleted QUICKSTART.md and two obsolete tags; mainpage links to pages that do not exist
severity: med | category: docs | effort: XS | fix-class: easy-unattended | verdict: CONFIRMED | status: fixed in bfba715
location: doc/Doxyfile:832-835, :270, :1134; doc/mainpage.md:9-14
## Problem
`INPUT` still has `QUICKSTART.md` (deleted in e6da8de), `TCL_SUBST` and `COLS_IN_ALPHA_INDEX` are obsolete, and `mainpage.md` links `readme.html` / `quickstart.html`. Doxygen names the README page `md__r_e_a_d_m_e.html`, so even the Readme link is dead. Together with the rest of the code this yields 77 warnings.
## Evidence
```
$ (cat doc/Doxyfile; echo OUTPUT_DIRECTORY=/tmp/x; echo HAVE_DOT=NO) | doxygen -     (doxygen 1.9.1, run from the repo root)
warning: Tag 'TCL_SUBST' at line 270 of file '-' has become obsolete.
warning: Tag 'COLS_IN_ALPHA_INDEX' at line 1134 of file '-' has become obsolete.
warning: tag INPUT: input source 'QUICKSTART.md' does not exist
warning: source 'QUICKSTART.md' is not a readable file or directory... skipping.
index.html: href="readme.html"  href="quickstart.html"   (both dead; only md__r_e_a_d_m_e.html exists)
77 warnings in total: 24 tb/*_tb.cpp (B-103), 24 mzi_active.h @param (B-102), 9 parse_element.cpp, 16 undocumented ctor params, 4 above
```
## Expected
No warning from configuration, working links.
## Suggested fix
```
doc/Doxyfile:832-835
 INPUT                  = src/ \
                          doc/mainpage.md \
-						 README.md \
-                         QUICKSTART.md
+                         README.md
doc/Doxyfile:270   delete the line `TCL_SUBST              =`
doc/Doxyfile:1134  delete the line `COLS_IN_ALPHA_INDEX    = 5`
doc/mainpage.md:9-14 ->
For now, you can have a look at:

* [Readme](README.md) to get some general information about the project

And refer to the auto-generated code documentation.
```
(`[Readme](README.md)` resolves to `md__r_e_a_d_m_e.html` in doxygen 1.9.1; drop the `**` bold inside the link text, it is rendered literally.) The Introduction TODO is B-084. `.gitlab-ci.yml` `doc`/`pages` expecting `doc/pcm-doc.tar.xz` is owned by B-120; the Meson `doc` run_target (PLAN.md:89) must run doxygen from the repo root because INPUT/OUTPUT_DIRECTORY are relative.
## Golden impact
none.
## Related
B-084, B-102, B-103, B-120.

- Discovery seeds: S3.05, S4.22, S5.09 (see `backlog/README.md` for seed numbering).
- Review ids: F-A6-08.
## Verification
Ran doxygen with the unchanged Doxyfile (output redirected to `/tmp/ws-review-A6/doxy2`), then a scratch tree with the proposed mainpage: link target resolved to `md__r_e_a_d_m_e.html`. May be partly absorbed by the Meson doc target, but the Doxyfile edits are needed either way.

**Independent second check (V5, of F-A6-08): CONFIRMED.** Method: ran doxygen 1.9.1 out of tree (OUTPUT_DIRECTORY=/tmp/ws-review-V5/doxy_*, HAVE_DOT=NO, from the tree root) before and after applying the edits

Before: 77 warnings, including TCL_SUBST obsolete (line 270), COLS_IN_ALPHA_INDEX obsolete (line 1134), "input source 'QUICKSTART.md' does not exist" and "not a readable file or directory". index.html has dead `readme.html` and `quickstart.html` hrefs. After the exact edits (INPUT without QUICKSTART.md and the tab-indented README line, both obsolete tags deleted, mainpage rewritten as suggested): 73 warnings; the 4 config warnings are the only difference. index.html now has `<a class="el" href="md__r_e_a_d_m_e.html">Readme</a>`, which exists. The edit is unique and local. Detail: the Doxyfile has a blank line after each of the two obsolete tags, so delete only the tag line (or the line plus its blank) consistently. Both are harmless. Fixer check: rerun doxygen out of tree as above and expect 73 warnings with no "Tag ... obsolete", "QUICKSTART" or dead-link lines. The Doxyfile must be run from the repo root because INPUT is relative.
