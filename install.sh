#!/bin/sh
sudo nix --extra-experimental-features "nix-command flakes" run nixpkgs#python313 install.py || exit 1
exit 0