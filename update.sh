#!/bin/sh
cd "$(dirname "$0")"
hostname="$(hostname)"
if ! sudo nix flake show ".#" | grep $hostname; then
  echo "Error: Hostname '$hostname' does not match any configuration in flake.nix"
  exit 1
fi
sudo nixos-rebuild switch --flake ".#$hostname"