#!/usr/bin/env bash
# The health a host reported on its first boot and on its second must be the same. Usage:
# compare_boots.sh <work directory>. Swap devices are listed in the order they came up, which is a race
# between the swap file and zram, so their order is not part of the comparison.
set -u
work=$1
for boot in 0 1; do
  grep -aE '^E2E-(STATE|FAILED|REFUSED|HOMEMANAGER|SNAPSHOTS|SWAP|UNLOCKAGENT)' "$work/boot-$boot.log" \
    | sed -E 's/^(E2E-UNLOCKAGENT [a-z0-9]+).*/\1/' \
    | while read -r tag rest; do
        if [ "$tag" = E2E-SWAP ]; then rest=$(printf '%s\n' $rest | sort | tr '\n' ' '); fi
        printf '%s %s\n' "$tag" "${rest% }"
      done | sort > "$work/norm-$boot.txt"
done
if diff "$work/norm-0.txt" "$work/norm-1.txt" > /dev/null; then
  printf boots-identical
else
  echo 'boots differ'
  diff "$work/norm-0.txt" "$work/norm-1.txt"
  exit 1
fi
