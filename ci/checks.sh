#!/usr/bin/env bash
# Builds every flake check of this system, one at a time, and names the ones that fail.
set -u
cd "$(dirname "$0")/.." || exit 1
system=$(nix eval --raw --impure --expr builtins.currentSystem)
names=$(nix eval --raw ".#checks.$system" --apply 'checks: builtins.concatStringsSep "\n" (builtins.attrNames checks)')
printf '%s\n' "$names"
failed=""
for name in $names; do
  printf '\n=== %s\n' "$name"
  nix build --no-link --print-build-logs ".#checks.$system.$name" || failed="$failed $name"
done
if [ -n "$failed" ]; then
  printf '\nfailed:%s\n' "$failed"
  exit 1
fi
