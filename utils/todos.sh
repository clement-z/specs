#!/bin/sh
# Print all TODOs found in the sources, utils/ and todo.txt (meson run_target `todos`,
# was `make todos`).
# Usage: todos.sh REPO_ROOT
cd "$1" || exit 1
todo_awk='/TODO:/ {split($0, arr, "TODO:"); content = arr[2]; print "- " content, "("FILENAME":"FNR")"}'
echo "TODO (from code):"
find src -type f | xargs awk "$todo_awk" || true
find utils -type f | xargs awk "$todo_awk" || true
cat todo.txt || true
