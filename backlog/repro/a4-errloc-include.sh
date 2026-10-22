#!/bin/bash
# A syntax error on line 3 of an included file is reported with the PARENT file name.
# usage: a4-errloc-include.sh <path/to/specs>
SPECS=${1:?usage: $0 <specs>}
D=$(mktemp -d); mkdir -p $D/traces; cd $D || exit 1
printf '* included file, syntax error on line 3\n\nwg1 a b ,\n' > child.cir
printf '* parent line 1\n* line 2\n* line 3\n* line 4\n.include child.cir\n.tran 1e-11\n' > parent.cir
timeout 60 "$SPECS" -f parent.cir -o traces/x 2>&1 | grep -A2 "Parsing error"
