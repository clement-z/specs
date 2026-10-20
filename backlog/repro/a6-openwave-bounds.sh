#!/usr/bin/env bash
# A6 repro: utils/openwave.sh bounds check uses ${#configs} (length of the first path) instead of ${#configs[@]}.
# usage: a6-openwave-bounds.sh <repo-root>
# Expected on the unfixed tree: choosing 9 with 3 configs prints 'Going with  (9).' and runs `gtkwave -a "" t.vcd`.
W=$(mktemp -d); mkdir -p "$W/cfg" "$W/bin"; touch "$W/cfg/a.gtkw" "$W/cfg/b.gtkw" "$W/cfg/c.gtkw" "$W/t.vcd"
printf '#!/bin/bash\necho "STUB gtkwave args: [$*]"\n' > "$W/bin/gtkwave"; chmod +x "$W/bin/gtkwave"
cd "$W" && printf '9\n' | PATH="$W/bin:$PATH" bash "${1:?repo root}/utils/openwave.sh" t.vcd cfg | tail -3
