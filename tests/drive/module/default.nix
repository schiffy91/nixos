{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  home = "/fixture/user";
  programs = import ../../../config/pkgs/utils/drive/package.nix { inherit pkgs inputs; };
  # The production module, evaluated for one user and pointed at the fake Drive. The units it
  # generates are what the VM boots with.
  profile = (inputs.nixpkgs.lib.nixosSystem {
    inherit system;
    specialArgs = { inherit inputs; };
    modules = [
      ../../../config/settings.nix
      ../../../config/pkgs/utils/drive
      ../../../config/system/storage/immutability
      {
        settings = {
          apps.utils.rclone.enable = false;
          apps.utils.drive = {
            enable = true;
            api = "http://127.0.0.1:18475";
            pinned = [ "Pinned" ];
            followSeconds = 1;
            quietSeconds = 1;
            readers.deny = [ "never-reads" ];
            freeFloorBytes = 0; # the test disk is small; with the default floor everything not pinned would be given back
          };
          users.admin = { username = "fixture"; homeDirectory = home; };
          users.agent.enable = false;
          disk = {
            encryption.enable = false;
            immutability = { enable = false; persist.neededForBoot = []; };
            subvolumes.volumes = [
              { name = "@fixture"; mountPoint = "/fixture"; resetOnBoot = true; }
              { name = "@snapshots"; mountPoint = "/.snapshots"; flag = "snapshots"; }
            ];
          };
        };
        networking.hostName = "module-test";
        users.users.fixture = { isNormalUser = true; inherit home; };
        system.stateVersion = "24.11";
      }
    ];
  }).config;
  unit = name: "${profile.systemd.units.${name}.unit}/${name}";
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "drive-module";
  nodes.machine = { ... }: {
    boot.kernelModules = [ "fuse" ];
    networking.hostName = "module-test";
    users.users.fixture = { isNormalUser = true; inherit home; uid = 1000; group = "users"; };
    systemd.tmpfiles.rules = [ "d /fixture 0755 root root -" ] ++ builtins.filter (pkgs.lib.hasInfix "/var/lib/drive") profile.systemd.tmpfiles.rules;
    environment.systemPackages = [ programs.drive programs.fake pkgs.curl pkgs.attr pkgs.python3 pkgs.libcap ];
    environment.sessionVariables = { inherit (profile.environment.sessionVariables) DRIVE_RUNTIME DRIVE_MOUNT; };
    systemd.packages = [ (pkgs.runCommand "drive-module-units" {} ''
      mkdir -p $out/lib/systemd/system
      cp ${unit "drive-door.service"} ${unit "drive-engine.service"} $out/lib/systemd/system/
    '') ];
    systemd.targets.multi-user.wants = [ "drive-door.service" "drive-engine.service" ];
    virtualisation.memorySize = 2048;
  };
  scenario = ./Main.btrc;
  extraFiles = [ ../Fixture.btrc ];
}
