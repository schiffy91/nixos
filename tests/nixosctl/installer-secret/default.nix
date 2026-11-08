{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  driver = pkgs.stdenv.mkDerivation {
    name = "installer-secret-test";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ../../../config/pkgs/utils/nixosctl/lib ../../../config/pkgs/utils/nixosctl/btrc.toml ../../../config/pkgs/utils/nixosctl/btrc.lock ./Driver.btrc ./btrc.toml ./btrc.lock ];
    };
    nativeBuildInputs = [ inputs.btrc.packages.${system}.btrcpy ];
    buildPhase = ''
      export HOME="$TMPDIR"
      btrcpy --strict-imports tests/nixosctl/installer-secret/Driver.btrc -o driver.c
      $CC -std=c11 -O2 driver.c -lm -lpthread -lutil -o driver
      btrcpy --strict-imports config/pkgs/utils/nixosctl/lib/PasswordHashFileMain.btrc -o password-file.c
      $CC -std=c11 -O2 password-file.c -lm -lpthread -o nixosctl-password-file
    '';
    installPhase = ''
      install -Dm755 driver $out/bin/installer-secret-test
      install -Dm755 nixosctl-password-file $out/bin/nixosctl-password-file
    '';
  };
  contracts = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "installer-secret-contracts";
    src = pkgs.lib.fileset.toSource {
      root = ../..;
      fileset = pkgs.lib.fileset.unions [ ../../lib ./GuestTests.btrc ];
    };
    programs.installer-secret-contracts = "nixosctl/installer-secret/GuestTests.btrc";
    linkFlags = [ "-lutil" ];
  };
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "installer-secret";
  nodes.machine = { ... }: {
    users.users.observer.isNormalUser = true;
    environment.systemPackages = [ driver contracts pkgs.mkpasswd ];
    virtualisation.memorySize = 1024;
  };
  scenario = ./Main.btrc;
}
