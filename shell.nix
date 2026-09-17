{ system ? builtins.currentSystem }:
let
  lock = builtins.fromJSON (builtins.readFile ./flake.lock);
  lockedSource = name:
    let node = lock.nodes.${name}.locked;
    in fetchTree {
      inherit (node) type owner repo rev narHash;
    };
  pkgs = import (lockedSource "nixpkgs") {
    inherit system;
    config.allowUnfree = true;
  };
  # The packaged compiler carries the native header reader, sysroot and target
  # that typed native bindings (btrc/tray/btrc.toml) need; the raw source does not.
  btrcNode = lock.nodes.btrc.locked;
  btrcFlake = builtins.getFlake "github:${btrcNode.owner}/${btrcNode.repo}/${btrcNode.rev}";
  btrcpy = btrcFlake.packages.${system}.btrcpy;
  btrc-lsp = btrcFlake.packages.${system}.btrc-lsp;
in
pkgs.mkShell {
  packages = [
    btrcpy
    btrc-lsp
    pkgs.gnumake
    pkgs.nixd
    pkgs.git
    pkgs.coreutils
    pkgs.stdenv.cc
    pkgs.pkg-config
    pkgs.dbus
  ];
}
