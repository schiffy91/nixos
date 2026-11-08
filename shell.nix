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
  # that typed native bindings (Library.Tray on libdbus) need; the raw source does not.
  btrcNode = lock.nodes.btrc.locked;
  btrcFlake = builtins.getFlake "github:${btrcNode.owner}/${btrcNode.repo}/${btrcNode.rev}";
  inherit (import ./config/pkgs/utils/nixosctl/ui/tray/lib.nix {
    inherit pkgs;
    inputs.btrc = btrcFlake;
  }) btrcpy btrc-lsp;
  nativeLibraries = [ pkgs.liblzf pkgs.zlib pkgs.curl pkgs.openssl ]
    ++ pkgs.lib.optionals pkgs.stdenv.hostPlatform.isLinux [ pkgs.cryptsetup pkgs.pam pkgs.libdrm ];
in
pkgs.mkShell {
  buildInputs = nativeLibraries;
  CPATH = pkgs.lib.makeSearchPath "include" (map pkgs.lib.getDev nativeLibraries);
  packages = [
    btrcpy
    btrc-lsp
    pkgs.gnumake
    (import ./config/pkgs/dev/nixd/package.nix { inherit pkgs; })
    pkgs.git
    pkgs.ripgrep
    pkgs.coreutils
    pkgs.stdenv.cc
    pkgs.pkg-config
    pkgs.dbus
  ];
}
