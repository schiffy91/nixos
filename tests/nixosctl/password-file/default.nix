{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  nixosctl = import (flake.outPath + "/nix/apps/utils/nixosctl/package.nix") {
    inherit pkgs;
    inherit (pkgs) lib;
    btrcpy = inputs.btrc.packages.${system}.btrcpy;
  };
  python = pkgs.python3.withPackages (p: [ p.pexpect ]);
in assert import ./storage.nix { inherit inputs system; };
pkgs.testers.runNixOSTest {
  name = "password-file";
  nodes.machine = { lib, ... }: {
    _module.args.inputs = inputs;
    imports = [ ../../../nix/settings.nix ../../../nix/system/nixos/sudo.nix ];
    settings.users.admin.username = "admin";
    users.mutableUsers = false;
    users.users.admin = {
      isNormalUser = true;
      extraGroups = [ "wheel" ];
      hashedPasswordFile = lib.mkDefault "/etc/nixos/secrets/hashed_password.txt";
    };
    system.activationScripts.legacyPassword = {
      deps = [ "specialfs" ];
      text = ''
        if ! test -e /etc/nixos/secrets/hashed_password.txt; then
          install -d -m 700 /etc/nixos/secrets
          echo testpass123 | ${pkgs.mkpasswd}/bin/mkpasswd -m sha-512 -s > /etc/nixos/secrets/hashed_password.txt
          chmod 600 /etc/nixos/secrets/hashed_password.txt
        fi
      '';
    };
    system.activationScripts.users.deps = [ "legacyPassword" ];
    specialisation.protected.configuration = {
      imports = [ ../../../nix/system/nixos/users/password.nix ];
    };
    security.sudo.extraConfig = "Defaults passwd_tries=1";
    environment.systemPackages = [ nixosctl python pkgs.mkpasswd ];
    environment.etc."password-file-test.py".source = ./test.py;
    virtualisation.memorySize = 1024;
    virtualisation.useBootLoader = true;
  };
  testScript = ''
    start_all()
    machine.wait_for_unit("multi-user.target")
    machine.succeed("${python}/bin/python3 ${./test.py}")
    machine.reboot()
    machine.wait_for_shutdown()
    machine.start()
    machine.wait_for_unit("multi-user.target")
    machine.succeed("${python}/bin/python3 ${./test.py} reboot")
  '';
}
