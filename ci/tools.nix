{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree { inherit (locked) type owner repo rev narHash; }) { inherit system; };
  compilerLock = (builtins.fromJSON (builtins.readFile ../flake.lock)).nodes.btrc.locked;
  compiler = builtins.getFlake (builtins.flakeRefToString (builtins.removeAttrs compilerLock [
    "narHash" "lastModified" "lastModifiedDate" "revCount" "dirtyRev" "dirtyShortRev"
  ]));
  package = (compiler.lib.buildPackageWith {
    inherit pkgs;
    inherit (compiler.packages.${system}) btrcpy btrc-native-plan;
  }) {
    name = "repo-tools";
    src = pkgs.lib.fileset.toSource { root = ../.; fileset = pkgs.lib.fileset.unions [ ./Main.btrc ./lib ./native.h ./btrc.toml ./btrc.lock ]; };
    programs.repo-tools = "ci/Main.btrc";
    runtimeInputs = [ pkgs.nix pkgs.git pkgs.coreutils ];
  };
in {
  inherit package;
  shell = (import ../shell.nix { inherit system; }).overrideAttrs (old: {
    nativeBuildInputs = [ package pkgs.shellcheck pkgs.nodejs pkgs.lua5_1 pkgs.actionlint pkgs.taplo pkgs.yq-go pkgs.darkhttpd ]  # WoW and CSP run Lua 5.1
      ++ old.nativeBuildInputs;
    buildInputs = old.buildInputs
      ++ [ pkgs.libsodium pkgs.libx11 pkgs.xorgproto pkgs.sdl3 pkgs.fontconfig pkgs.freetype compiler.packages.${system}.wgpu-native ];  # headers deckpad and drive bind
  });
}
