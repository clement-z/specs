#!/bin/bash
# F-A7-02: `%option stack` makes flex emit an unused yy_top_state() (-Wunused-function in scanner.cpp);
# `%option noyy_top_state` removes exactly that function and nothing else.
# usage: a7-flex-top-state.sh <repo>
REPO=${1:?repo}; W=$(mktemp -d /tmp/a7-flex.XXXXXX); cd "$W" || exit 1
cp "$REPO/src/parser/parser.l" o.l; sed 's/^%option stack$/%option stack\n%option noyy_top_state/' o.l > p.l
flex --header-file=o.h -o o.cpp o.l 2>/dev/null; flex --header-file=p.h -o p.cpp p.l 2>/dev/null
diff <(sed 's/#line [0-9]* "[^"]*"//' o.cpp) <(sed 's/#line [0-9]* "[^"]*"//' p.cpp)
rm -rf "$W"
