#!/bin/sh
# A2 repro: --vci printing segfaults when a port is bound to a channel that is not
# spx::oa_signal_type (CROW uses sc_signal<OpticalSignal> with the default writer policy).
# Run from a dir containing traces/. Observed: exit 139 right after "phase delay = ... (1.5708rad @1.55)".
# Without --vci the same testbench exits 0.
mkdir -p traces
specs --vci -t ac_crow; echo "exit=$?"
