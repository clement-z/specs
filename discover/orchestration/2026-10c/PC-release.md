# Phase C — v0.2.0 release preparation (coordinator; stop before tag/merge)

Prerequisites: phase A merged, phase B green (all CI jobs, sanitizer job blocking). Release content = sprints 2026-10,
2026-10b and phase A (see `backlog/DECISIONS-2026-10-21.md`, "Versioning").

1. `meson.build`: `version: '0.2.0'` (was `0.2.0-dev`). `utils/git-version.sh` keeps deriving `--version` from the
   tag once the maintainer tags `v0.2.0`.
2. `CHANGELOG.md`: rename `[unreleased]` to `[0.2.0] - <release date>`, regroup under New / Changed (incl. output
   changes users will notice: field label √W, `.dc` stop point, tracing only probes when present, PCMCELL `tc`/`ta`
   required, `--mon` removed, positional argument order) / Fixed / Removed; start a new empty `[unreleased]`.
3. `CITATION.cff` (CFF 1.2.0) for the software, `preferred-citation` = the ECOC 2023 paper (already in the README);
   `.zenodo.json` with the same metadata (scout F-20 brief). **Needs from the maintainer**: author list, ORCIDs,
   affiliations, keywords; whether the GitHub–Zenodo integration is enabled.
4. README: "How to cite" section pointing to `CITATION.cff` (software DOI added after Zenodo mints it).
5. Full gate on the release commit (gcc, clang, clang ASan; golden; CI green after an explicitly requested push).
6. **Stop.** Report the commit to tag; the maintainer requests the tag, the merge into `dev`/`master` and the push.
