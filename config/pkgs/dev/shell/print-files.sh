#!/usr/bin/env bash
set -euo pipefail

if (( $# < 1 )); then
    echo 'Usage: print-files <directory> [patterns ...]' >&2
    exit 1
fi

directory=$1
shift
if (( $# == 0 )); then set -- '*'; fi
# Keep relative paths beginning with '-' out of find's expression syntax.
case "$directory" in
    -*|'!'|'(') directory="./$directory" ;;
esac

for pattern in "$@"; do
    find "$directory" -type f -name "$pattern" -print0 |
        while IFS= read -r -d '' file; do
            printf '==File: %s==\n' "$file"
            cat -- "$file"
            printf '\n'
        done
done
