#!/bin/sh
# `.dc` sweep end point depends on floating-point accumulation in range() (src/utils/general_utils.h:21-33).
# usage: sh a5-dc-sweep-endpoint.sh /path/to/specs
SPECS=${1:?specs binary}
W=$(mktemp -d); trap 'rm -rf "$W"' EXIT; cd "$W"; mkdir traces
cat > a.cir <<'N'
cwsrc1 in wl=1.55e-6 power=1
probe1 in
.assign lambda0 = 1.55e-6
.assign lambda_min = {lambda0} - 15e-9
.assign lambda_max = 2*{lambda0}-{lambda_min}
.dc /cwsrc1/WL {lambda_min} {lambda_max} 1e-9
N
cat > b.cir <<'N'
cwsrc1 in wl=1.55e-6 power=1
probe1 in
.dc /cwsrc1/WL 1.0e-6 2.0e-6 0.1e-6
N
for f in a b; do echo "== $f.cir"; "$SPECS" -f $f.cir -o traces/$f 2>&1 | grep -E "Starting sweep|emitted" | sed -n '1p;$p'; done
echo "expected (inclusive stop, start+i*step): a = 31 points ending at 1565 nm, b = 11 points ending at 2000 nm"
echo "observed: a = 30 points, last 1564 nm (stop dropped); b = 11 points, last value 1.999999999999999e-6 (stop kept only because the accumulated sum is 1 ulp short)"
