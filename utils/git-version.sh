#!/bin/sh
# Print the SPECS version from the git tags, like the former cmake/version.cmake:
#   full                 the latest tag on a tagged commit, else <tag>-<commits since tag>.g<short hash>
#   major|minor|patch    the components of a vX.Y.Z tag (0 when the tag is not of that form)
# Fails (no output) outside a git checkout or without tags; Meson then falls back to the project version.
src=${1:?usage: git-version.sh <srcdir> full|major|minor|patch}
what=${2:-full}
tag=$(git -C "$src" describe --tags --abbrev=0 2>/dev/null) || exit 1
case $what in
    full)
        n=$(git -C "$src" rev-list "$tag"..HEAD --count) || exit 1
        if [ "$n" = 0 ]; then
            echo "$tag"
        else
            echo "$tag-$n.g$(git -C "$src" rev-parse --short HEAD)"
        fi
        ;;
    major|minor|patch)
        set -- $(echo "$tag" | sed -n 's/^v\([0-9][0-9]*\)\.\([0-9][0-9]*\)\.\([0-9][0-9]*\).*/\1 \2 \3/p')
        case $what in
            major) echo "${1:-0}" ;;
            minor) echo "${2:-0}" ;;
            patch) echo "${3:-0}" ;;
        esac
        ;;
    *)
        exit 2
        ;;
esac
