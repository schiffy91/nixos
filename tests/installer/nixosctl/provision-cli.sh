#!/usr/bin/env bash
set -euo pipefail
work_dir=$(realpath "$1")
port=$2
case "$3" in
    x86_64|aarch64) system="$3-linux" ;;
    *) echo 'unsupported guest architecture' >&2; exit 1 ;;
esac

# Exercise the packaged CLI in the guest; compiling the developer shell there
# needlessly downloads the language server, editor tooling and extra compilers.
package=$(nix-build --no-out-link --cores 4 --max-jobs 2 \
    --argstr root "$PWD" --argstr system "$system" --expr '
    { root, system }:
    let
      flake = builtins.getFlake ("git+file://" + root);
      pkgs = import flake.inputs.nixpkgs { inherit system; };
    in import (flake.outPath + "/config/pkgs/utils/nixosctl/package.nix") {
      inherit pkgs;
      inherit (pkgs) lib;
      btrcpy = flake.inputs.btrc.packages.${system}.btrcpy;
    }')

ssh_options=(-i "$work_dir/id_ed25519" -p "$port" -o BatchMode=yes
    -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o ConnectTimeout=10)
printf -v NIX_SSHOPTS '%q ' "${ssh_options[@]}"
export NIX_SSHOPTS
nix-copy-closure --to root@127.0.0.1 "$package"
ssh "${ssh_options[@]}" root@127.0.0.1 "ln -s '$package/bin/nixosctl' /run/nixosctl-under-test"
