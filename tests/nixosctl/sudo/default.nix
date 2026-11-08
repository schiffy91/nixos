{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  nixosctl = import (flake.outPath + "/config/pkgs/utils/nixosctl/package.nix") {
    inherit pkgs;
    inherit (pkgs) lib;
    btrcpy = inputs.btrc.packages.${system}.btrcpy;
  };
  python = pkgs.python3.withPackages (p: [ p.pexpect ]);
in pkgs.testers.runNixOSTest {
  name = "nixosctl-sudo";
  nodes.machine = { ... }: {
    _module.args.inputs = inputs;
    imports = [
      ../../../config/settings.nix
      ../../../config/system/nixos/sudo.nix
      ../../../config/system/nixos/networking.nix
    ];
    settings.users.admin.username = "admin";
    settings.networking.forwardAgentHosts = [ "TRUSTED.example" "192.0.2.1" ];
    users.users.admin = {
      isNormalUser = true;
      extraGroups = [ "wheel" ];
      password = "testpass123";
    };
    users.users.agent.isNormalUser = true;
    security.sudo.extraConfig = "Defaults passwd_tries=1";
    environment.systemPackages = [ nixosctl python pkgs.openssh ];
    virtualisation.memorySize = 1024;
  };
  testScript = ''
    start_all()
    machine.wait_for_unit("multi-user.target")
    machine.succeed("${python}/bin/python3 ${./test.py}")
    machine.succeed("nixosctl sudo enable --for 1h")
    machine.reboot()
    machine.wait_for_shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target")
    machine.succeed("test ! -e /run/nixosctl-sudo/grant")
    machine.fail("su -s /bin/sh admin -c 'sudo -n -k true'")
    machine.succeed("nixosctl sudo status | grep 'No temporary sudo grant'")
  '';
}
