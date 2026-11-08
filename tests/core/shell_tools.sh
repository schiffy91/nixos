#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "$0")/../.." && pwd)
print_files=${PRINT_FILES:-$root/config/pkgs/dev/shell/print-files.sh}
extract_substring=${EXTRACT_SUBSTRING:-$root/config/pkgs/dev/shell/extract-substring.sh}
temporary=$(mktemp -d)
trap 'rm -rf "$temporary"' EXIT

extract() {
    printf '%s' "$3" > "$temporary/input"
    bash "$extract_substring" "$1" "$2" "$temporary/input"
}

[[ $(extract '[' ']' $'[one]\nignored\n[two]\n') == $'one\ntwo' ]]
[[ $(extract '[' ']' 'before[one]after') == 'beforeoneafter' ]]
[[ $(extract '[' ']' '[one][two]') == 'one][two' ]]
[[ $(extract 'a.b' '(end)' 'a.bhello(end)') == 'hello' ]]
[[ $(extract '*' '?' 'before*hello?after') == 'beforehelloafter' ]]
[[ $(extract '[' ']' $'[one]\n[]\n[two]\n[]') == $'one\n\ntwo' ]]
[[ $(extract '[' ']' $'[one] \t') == $'one \t' ]]
[[ $(extract '' '' 'hello') == 'hello' ]]
if extract '[' ']' 'missing' 2>/dev/null; then exit 1; fi
if extract '[' ']' '[]' 2>/dev/null; then exit 1; fi
if bash "$extract_substring" '[' ']' "$temporary/missing" 2>/dev/null; then exit 1; fi
if bash "$extract_substring" 2>/dev/null; then exit 1; fi

mkdir -p "$temporary/files/nested"
file="$temporary/files/nested/quoted ' file"$'\n.bin'
printf 'before\0after\n' > "$file"
printf 'skip' > "$temporary/files/other.txt"
ln -s nested "$temporary/files/linked-directory"
ln -s "$file" "$temporary/files/linked-file.bin"
{
    printf '==File: %s==\n' "$file"
    cat -- "$file"
    printf '\n'
} > "$temporary/expected"
bash "$print_files" "$temporary/files" '*.bin' > "$temporary/actual"
cmp "$temporary/expected" "$temporary/actual"
bash "$print_files" "$file" > "$temporary/actual"
cmp "$temporary/expected" "$temporary/actual"
bash "$print_files" "$temporary/files" '*.bin' '*.txt' > "$temporary/actual"
{
    printf '==File: %s==\n' "$temporary/files/other.txt"
    printf 'skip\n'
} >> "$temporary/expected"
cmp "$temporary/expected" "$temporary/actual"
if bash "$print_files" "$temporary/missing" 2>/dev/null; then exit 1; fi
if bash "$print_files" 2>/dev/null; then exit 1; fi
if [[ -e /dev/full ]]; then
    if bash "$print_files" "$file" > /dev/full 2>/dev/null; then exit 1; fi
fi

echo 'PASS: shell helpers, literal delimiters, whitespace, recursive patterns, binary output, symlinks and failures'
