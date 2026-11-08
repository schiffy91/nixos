{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
, pkgs ? import inputs.nixpkgs { inherit system; }
}:
let
  build = import ../../../config/lib/btrc.nix { inherit pkgs inputs; };
  tools = build {
    name = "gaming-runtime-test-tools";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [
        ../../../config/pkgs/gaming/sunshine/credentials/src
        ../../../config/pkgs/gaming/sunshine/virtual-display
        ../../../config/pkgs/gaming/slopsmith/src
        ../../../config/lib/BackgroundProcess.btrc
        ./. ../../lib
      ];
    };
    programs = {
      contracts = "tests/gaming/runtime/Contracts.btrc";
      integration = "tests/gaming/runtime/Integration.btrc";
      login-server = "tests/gaming/runtime/LoginServer.btrc";
      command-fixture = "tests/gaming/runtime/CommandFixture.btrc";
      credentials = "config/pkgs/gaming/sunshine/credentials/src/Main.btrc";
      virtual-display = "config/pkgs/gaming/sunshine/virtual-display/Main.btrc";
      slopsmith-runtime = "config/pkgs/gaming/slopsmith/src/Main.btrc";
    };
    buildInputs = [ pkgs.openssl ];
    linkFlags = [ "-lssl" "-lcrypto" ];
  };
  ssh = import ../../../config/pkgs/gaming/sunshine/ssh-login/package.nix { inherit pkgs inputs; };
in pkgs.runCommand "gaming-runtime-contracts" {
  nativeBuildInputs = [ pkgs.openssl pkgs.openssh pkgs.coreutils pkgs.bash pkgs.curl ];
} ''
  ${tools}/bin/contracts
  ${tools}/bin/integration ${tools}/bin ${ssh}/bin/sunshine-ssh-login
  touch "$out"
''
