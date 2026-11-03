#!/usr/bin/env bash
# Bootstrap a host from the NixOS installer ISO:
#   rm -rf /etc/nixos && git clone https://github.com/schiffy91/nixos /etc/nixos && /etc/nixos/install.sh
set -euo pipefail

[ "$(id -u)" = 0 ] || exec sudo -- "$0" "$@"

cd /etc/nixos
export NIX_CONFIG='experimental-features = nix-command flakes'
nix-shell --run 'make build'

exec build/nixosctl install "$@"  # prompts before it formats anything
