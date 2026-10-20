#!/usr/bin/env bash
# Trace helpers (meson run_targets, were Makefile targets of the same name).
# Usage: traces.sh cleantraces|cleanoldtraces|waves|newwaves TRACES_DIR
set -e
action=$1
traces=$2
here=$(dirname "$0")

# Latest trace by modification time (same as the Makefile's LATEST_TRACE_NAME)
latest() {
    ls -1rt --group-directories-first "$traces" | tail -1
}

case "$action" in
    cleantraces)
        echo "Removing all traces"
        find "$traces/" -maxdepth 1 -type f -and -iname '*.vcd' -delete
        ;;
    cleanoldtraces)
        echo "Removing all traces but newest"
        find "$traces/" -maxdepth 1 -iname '*.vcd' -and -type f -and ! -iname "$(latest)" -delete
        ;;
    newwaves)
        # Show the latest trace with a blank GTKWave config
        echo "Opening latest trace file"
        mkdir -p "$traces/configs"
        bash "$here/openwave.sh" "$traces/$(latest)"
        ;;
    waves)
        # Show the latest trace with a menu to select which GTKWave config to use
        echo "Opening latest trace file"
        mkdir -p "$traces/configs"
        bash "$here/openwave.sh" "$traces/$(latest)" "$traces/configs"
        ;;
    *)
        echo "usage: $0 cleantraces|cleanoldtraces|waves|newwaves TRACES_DIR" >&2
        exit 2
        ;;
esac
