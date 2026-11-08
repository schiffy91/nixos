#!/usr/bin/env bash
set -euo pipefail

if (( $# != 3 )); then
    echo 'Usage: extract-substring <prefix> <suffix> <file>' >&2
    exit 1
fi

prefix=$1
suffix=$2
result=''
while IFS= read -r line || [[ -n "$line" ]]; do
    [[ "$line" == *"$prefix"* ]] || continue
    before=${line%%"$prefix"*}
    rest=${line#*"$prefix"}
    [[ "$rest" == *"$suffix"* ]] || continue
    # Preserve the historical behavior: remove the first prefix and last suffix,
    # keeping the text outside them as well as the text between them.
    middle=${rest%"$suffix"*}
    after=${rest##*"$suffix"}
    result+="$before$middle$after"$'\n'
done < "$3"

while [[ "$result" == *$'\n' ]]; do result=${result%$'\n'}; done
if [[ -z "$result" ]]; then
    printf 'Substring not found between prefix "%s" and suffix "%s".\n' "$prefix" "$suffix" >&2
    exit 1
fi
printf '%s\n' "$result"
