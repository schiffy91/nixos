#!/bin/sh
cd "$(dirname "$0")"
hostname="$(hostname)"
sudo nixos-rebuild switch --flake ".#$hostname"