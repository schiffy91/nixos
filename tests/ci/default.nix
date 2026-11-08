{ system ? builtins.currentSystem }:
let
  lock = (builtins.fromJSON (builtins.readFile ../../flake.lock)).nodes;
  pkgs = import (builtins.fetchTree { inherit (lock.nixpkgs.locked) type owner repo rev narHash; }) { inherit system; };
  compiler = builtins.getFlake "github:${lock.btrc.locked.owner}/${lock.btrc.locked.repo}/${lock.btrc.locked.rev}";
  programs = (import ../../config/lib/btrc.nix { inherit pkgs; btrcpy = compiler.packages.${system}.btrcpy; }) {
    name = "ci-contract-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../..;
      fileset = pkgs.lib.fileset.unions [ ../../ci/Main.btrc ../../ci/lib ./ToolsTests.btrc ./PipeTests.btrc ./NativeTests.btrc ];
    };
    programs = {
      repo-tools = "ci/Main.btrc";
      frames-test = "tests/ci/ToolsTests.btrc";
      pipes-test = "tests/ci/PipeTests.btrc";
      native-test = "tests/ci/NativeTests.btrc";
    };
  };
  nixd = pkgs.callPackage ../../config/pkgs/dev/nixd/package.nix {};
  inherit (compiler.packages.${system}) btrc-lsp;
in pkgs.runCommand "ci-contract-check" { nativeBuildInputs = [ pkgs.python3 pkgs.coreutils ]; } ''
  ${programs}/bin/frames-test
  ${programs}/bin/pipes-test
  python3 ${./publish.py} ${programs}/bin/repo-tools
  touch flake.nix
  ${programs}/bin/repo-tools nix-builtins ${nixd}/bin/nixd
  cp -r ${./native-fixture} native-fixture
  chmod -R u+w native-fixture
  ${programs}/bin/native-test ${btrc-lsp}/bin/btrc-lsp "$PWD/native-fixture/Valid.btrc"
  touch "$out"
''
