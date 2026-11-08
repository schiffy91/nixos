{ flake ? builtins.getFlake "git+file://${toString ../../..}"
, inputs ? flake.inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  nixosctl = import ../../../config/pkgs/utils/nixosctl/package.nix {
    inherit pkgs;
    inherit (pkgs) lib;
    btrcpy = inputs.btrc.packages.${system}.btrcpy;
  };
  contracts = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "password-file-contracts";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [ ../../lib ../../../config/lib/BackgroundProcess.btrc
        ../../../config/pkgs/utils/nixosctl ./GuestTests.btrc ./btrc.toml ./btrc.lock ];
    };
    programs.password-file-contracts = "tests/nixosctl/password-file/GuestTests.btrc";
    linkFlags = [ "-lutil" ];
  };
in assert import ./storage.nix { inherit inputs system; };
(import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "password-file";
  nodes.machine = { lib, ... }: {
    _module.args.inputs = inputs;
    imports = [ ../../../config/settings.nix ];
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
      imports = [ ../../../config/system/nixos/users/password.nix ];
    };
    security.sudo.extraConfig = "Defaults passwd_tries=1";
    environment.systemPackages = [ nixosctl contracts pkgs.mkpasswd pkgs.perl ];
    virtualisation.memorySize = 1024;
    virtualisation.useBootLoader = true;
  };
  scenario = ./Main.btrc;
}
