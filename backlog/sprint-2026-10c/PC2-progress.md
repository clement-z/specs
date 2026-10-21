# PC2 progress — SystemC as a Meson subproject, thirdparty/ → subprojects/

Heartbeat: `c423b484` (PC2-heartbeat, */5, expires in 48h).
Branch: `ws/subprojects`. Scope: "Decisions of 2026-10-21, third round" (backlog/DECISIONS-2026-10-21.md).

| Step | Status | Commit | Notes |
|---|---|---|---|
| 1. Layout (thirdparty/ → subprojects/, submodules removed) | done | a13e765 | |
| 2. Wraps (systemc, args), tarball diff vs submodule | done | a13e765 | args identical; SystemC tarball lacks only export-ignored dotfiles |
| 3. SystemC/args from subproject by default | done | 5c9b535 | force_fallback_for=systemc,args; nofallback wins; tested on Meson 1.3.0 + 1.11.2 |
| 4. CI and scripts | done | 1cddeee, 27afe84 | ccache + packagecache caches; build-systemc.sh removed |
| 5. Docs | done | 5b62394 | discover/0*.md, STATUS.md left as historical |
| 3b. SystemC subproject not sanitized | done | 65da3ff | UBSan vptr in SystemC process dispatch made ASan builds fail |
| Checks | done | | all pass; see PC2-REPORT.md |
| Report | done | | PC2-REPORT.md; heartbeat c423b484 deleted |
