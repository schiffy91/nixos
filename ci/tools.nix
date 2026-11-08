{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree { inherit (locked) type owner repo rev narHash; }) { inherit system; };
  python = pkgs.python3.withPackages (p: [ p.pillow p.pexpect p.vncdotool ]);
  compilerLock = (builtins.fromJSON (builtins.readFile ../flake.lock)).nodes.btrc.locked;
  compiler = builtins.getFlake "github:${compilerLock.owner}/${compilerLock.repo}/${compilerLock.rev}";
  package = (import ../config/lib/btrc.nix {
    inherit pkgs;
    btrcpy = compiler.packages.${system}.btrcpy;
  }) {
    name = "repo-tools";
    src = pkgs.lib.fileset.toSource { root = ../.; fileset = pkgs.lib.fileset.unions [ ./Main.btrc ./lib ]; };
    programs.repo-tools = "ci/Main.btrc";
    runtimeInputs = [ pkgs.nix pkgs.git pkgs.coreutils ];
  };
in {
  inherit python package;
  shell = (import ../shell.nix { inherit system; }).overrideAttrs (old: {
    nativeBuildInputs = [ package python pkgs.ruff pkgs.pyright pkgs.shellcheck pkgs.nodejs pkgs.lua pkgs.actionlint pkgs.taplo pkgs.yq-go pkgs.darkhttpd ]
      ++ old.nativeBuildInputs;
    CI_PYTHON = "${python}/bin/python3";
    PYTHONPYCACHEPREFIX = "/tmp/nixos-python-cache";
  });
}
