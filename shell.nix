{ system ? builtins.currentSystem }:
let
  lock = builtins.fromJSON (builtins.readFile ./flake.lock);
  lockedSource = name:
    let node = lock.nodes.${name}.locked;
    in builtins.fetchTree {
      inherit (node) type owner repo rev narHash;
    };
  pkgs = import (lockedSource "nixpkgs") {
    inherit system;
    config.allowUnfree = true;
  };
  btrcSource = lockedSource "btrc";
  btrcPython = pkgs.python314.withPackages (ps: [
    ps.pygls
    ps.lsprotocol
  ]);
  btrcpy = pkgs.writeShellApplication {
    name = "btrcpy";
    runtimeInputs = [ btrcPython ];
    text = ''
      export PYTHONPATH="${btrcSource}''${PYTHONPATH:+:$PYTHONPATH}"
      exec ${btrcPython}/bin/python3 -m src.compiler.python.main "$@"
    '';
  };
  btrc-lsp = pkgs.writeShellApplication {
    name = "btrc-lsp";
    runtimeInputs = [ btrcPython ];
    text = ''
      export PYTHONPATH="${btrcSource}''${PYTHONPATH:+:$PYTHONPATH}"
      exec ${btrcPython}/bin/python3 -m src.devex.lsp.server "$@"
    '';
  };
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
  ];
}
