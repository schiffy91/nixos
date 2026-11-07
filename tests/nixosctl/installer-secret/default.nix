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
      fileset = pkgs.lib.fileset.unions [ ../../../btrc ./Driver.btrc ];
    };
    nativeBuildInputs = [ inputs.btrc.packages.${system}.btrcpy ];
    buildPhase = ''
      export HOME="$TMPDIR"
      btrcpy --strict-imports tests/nixosctl/installer-secret/Driver.btrc -o driver.c
      $CC -std=c11 -O2 driver.c -lm -lpthread -lutil -o driver
    '';
    installPhase = "install -Dm755 driver $out/bin/installer-secret-test";
  };
  python = pkgs.python3.withPackages (p: [ p.pexpect ]);
in pkgs.testers.runNixOSTest {
  name = "installer-secret";
  nodes.machine = { ... }: {
    users.users.observer.isNormalUser = true;
    environment.systemPackages = [ driver python pkgs.mkpasswd ];
    virtualisation.memorySize = 1024;
  };
  testScript = ''
    start_all()
    machine.wait_for_unit("multi-user.target")
    machine.succeed("${python}/bin/python3 ${./test.py}")
    machine.reboot()
    machine.wait_for_shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target")
    machine.succeed("test ! -e /run/installer-test/killed/passphrase")
  '';
}
