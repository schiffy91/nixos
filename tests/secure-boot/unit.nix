{ flake ? builtins.getFlake "git+file://${toString ../..}", inputs ? flake.inputs, system ? builtins.currentSystem }:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  tests = (import ../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "secure-boot-key-tests";
    src = pkgs.lib.fileset.toSource {
      root = ../..;
      fileset = pkgs.lib.fileset.unions [
        ../../config/pkgs/utils/nixosctl/secure-boot-keys
        ./ArchiveTests.btrc ./CryptoTests.btrc ./btrc.toml
      ];
    };
    programs = {
      key-archive-tests = "tests/secure-boot/ArchiveTests.btrc";
      key-crypto-tests = "tests/secure-boot/CryptoTests.btrc";
    };
    buildInputs = [ pkgs.openssl ];
    linkFlags = [ "-lcrypto" ];
  };
in pkgs.runCommand "secure-boot-key-contracts" {
  nativeBuildInputs = [ pkgs.python3 pkgs.openssl ];
} ''
  ${tests}/bin/key-archive-tests
  python3 ${./crypto.py} ${tests}/bin/key-crypto-tests
  touch "$out"
''
