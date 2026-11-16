#!/usr/bin/env bash
# Builds every flake check of this system in one nix invocation and names the ones that fail.
# A list that cannot be read, or is empty, is a failure: nothing built must never read as a pass.
set -u
cd "$(dirname "$0")/.." || exit 1
system=$(nix eval --raw --impure --expr builtins.currentSystem) || { printf 'cannot read the current system\n' >&2; exit 1; }
names=$(nix eval --raw ".#checks.$system" --apply 'checks: builtins.concatStringsSep "\n" (builtins.attrNames checks)') || { printf 'cannot list the checks of %s\n' "$system" >&2; exit 1; }
[ -n "$names" ] || { printf 'no checks are listed for %s\n' "$system" >&2; exit 1; }
printf '%s\n' "$names"
installables=()
for name in $names; do installables+=(".#checks.$system.$name"); done
nix build --no-link --print-build-logs --keep-going "${installables[@]}" && exit 0
failed=""
for name in $names; do  # everything that passed is already built, so this only names the failures
  nix build --no-link ".#checks.$system.$name" > /dev/null 2>&1 || failed="$failed $name"
done
printf '\nfailed:%s\n' "$failed"
exit 1
