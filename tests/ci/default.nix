{ system ? builtins.currentSystem }:
let
  lock = (builtins.fromJSON (builtins.readFile ../../flake.lock)).nodes;
  pkgs = import (builtins.fetchTree { inherit (lock.nixpkgs.locked) type owner repo rev narHash; }) { inherit system; };
  compiler = builtins.getFlake "github:${lock.btrc.locked.owner}/${lock.btrc.locked.repo}/${lock.btrc.locked.rev}";
  programs = (import ../../config/lib/btrc.nix { inherit pkgs; btrcpy = compiler.packages.${system}.btrcpy; }) {
    name = "ci-contract-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../..;
      fileset = pkgs.lib.fileset.unions [ ../../ci/Main.btrc ../../ci/lib ../../ci/native.h ../../ci/btrc.toml ../../ci/btrc.lock ../lib ./ToolsTests.btrc ./PipeTests.btrc ./NativeTests.btrc ./PublishTests.btrc ./PublishFixture.btrc ./ShutdownTests.btrc ./ShutdownServer.btrc ./ProcessProbe.btrc ./process-probe.h ./btrc.toml ./btrc.lock ];
    };
    programs = {
      repo-tools = "ci/Main.btrc";
      frames-test = "tests/ci/ToolsTests.btrc";
      pipes-test = "tests/ci/PipeTests.btrc";
      native-test = "tests/ci/NativeTests.btrc";
      publish-test = "tests/ci/PublishTests.btrc";
      publish-fixture = "tests/ci/PublishFixture.btrc";
      shutdown-test = "tests/ci/ShutdownTests.btrc";
      shutdown-server = "tests/ci/ShutdownServer.btrc";
    };
  };
  nixd = pkgs.callPackage ../../config/pkgs/dev/nixd/package.nix {};
  inherit (import ../../config/pkgs/dev/btrc/compat.nix {
    inherit pkgs;
    packages = compiler.packages.${system};
  }) btrc-lsp;
in pkgs.runCommand "ci-contract-check" { nativeBuildInputs = [ pkgs.coreutils ]; } ''
  ${programs}/bin/frames-test
  ${programs}/bin/pipes-test
  ${programs}/bin/shutdown-test ${programs}/bin/repo-tools ${programs}/bin/shutdown-server
  ${programs}/bin/publish-test ${programs}/bin/repo-tools ${programs}/bin/publish-fixture
  # nixd's evaluator initializes profile state, even for the empty attrset.
  # Give it writable test state instead of the sandbox's read-only /nix/var.
  export NIX_STATE_DIR="$TMPDIR/var/nix"
  ${nixd}/libexec/nixd-attrset-eval < /dev/null
  touch flake.nix
  ${programs}/bin/repo-tools nix-builtins ${nixd}/bin/nixd || {
    cat build/lint/nix-regression-server.log
    exit 1
  }
  cp -r ${./native-fixture} native-fixture
  chmod -R u+w native-fixture
  ${programs}/bin/native-test ${btrc-lsp}/bin/btrc-lsp "$PWD/native-fixture/Valid.btrc"
  touch "$out"
''
