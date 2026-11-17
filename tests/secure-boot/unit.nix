{ flake ? builtins.getFlake "git+file://${toString ../..}", inputs ? flake.inputs, system ? builtins.currentSystem }:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  tests = (inputs.btrc.lib.buildPackage pkgs) {
    name = "secure-boot-key-tests";
    src = (import ../lib/test-source.nix { inherit pkgs inputs; }) {
      root = ../..;
      fileset = pkgs.lib.fileset.unions [
        ../../config/pkgs/utils/nixosctl/secure-boot-keys
        ../../config/pkgs/utils/nixosctl/lib/hardware/SecureBootVerification.btrc
        ./ArchiveTests.btrc ./CryptoTests.btrc ./CryptoIntegration.btrc ./AlarmState.btrc ./alarm-state.h ./VerificationTests.btrc ./btrc.toml ./btrc.lock
      ];
    };
    programs = {
      key-archive-tests = "tests/secure-boot/ArchiveTests.btrc";
      key-crypto-tests = "tests/secure-boot/CryptoTests.btrc";
      key-crypto-integration = "tests/secure-boot/CryptoIntegration.btrc";
      verification-tests = "tests/secure-boot/VerificationTests.btrc";
    };
    buildInputs = [ pkgs.openssl ];
  };
in pkgs.runCommand "secure-boot-key-contracts" {
  nativeBuildInputs = [ pkgs.openssl ];
} ''
  ${tests}/bin/key-archive-tests
  ${tests}/bin/verification-tests
  ${tests}/bin/key-crypto-integration ${tests}/bin/key-crypto-tests
  touch "$out"
''
