#!/bin/bash
# F-A7-10: root .gitignore ignores every *.cir (circuits/ files were force-added) and the unanchored `specs`
# pattern ignores any file or directory named "specs" at any depth.
# usage: a7-gitignore-cir.sh <repo>
REPO=${1:?repo path}; cd "$REPO" || exit 1
git check-ignore -v circuits/new_example.cir pyspecs/specs/x.py src/specs/a.cpp
echo "tracked files that match an ignore pattern:"; git ls-files -ci --exclude-standard
