{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  inherit (pkgs) lib;
  home = "/fixture/user";
  # The production rclone module, evaluated for one offline folder. Its generated user units
  # are what the VM runs; "gdrive:" is an alias for a local folder standing in for the Drive.
  profile = (inputs.nixpkgs.lib.nixosSystem {
    inherit system;
    specialArgs = { inherit inputs; };
    modules = [
      ../../../config/settings.nix
      ../../../config/pkgs/utils/rclone.nix
      ../../../config/system/storage/immutability
      {
        settings = {
          apps.utils.rclone.enable = true;
          apps.utils.rclone.offline.library = {
            remote = "media/Library";
            local = "${home}/Library";
            marker = ".semu-layout";
            exclude = [ "/_legacy/**" "*.semu-partial" ];
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
        users.users.fixture = { isNormalUser = true; inherit home; };
        system.stateVersion = "24.11";
      }
    ];
  }).config;
  unit = name: "${profile.systemd.user.units.${name}.unit}/${name}";
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "rclone-offline-folder";
  nodes.machine = { ... }: {
    users.users.fixture = { isNormalUser = true; inherit home; uid = 1000; linger = true; };
    systemd.tmpfiles.rules = [ "d /fixture 0755 root root -" ];
    environment.etc = {
      "offline-folder/rclone-offline-library.service".source = unit "rclone-offline-library.service";
      "offline-folder/rclone-offline-library.timer".source = unit "rclone-offline-library.timer";
      "offline-folder/rclone.conf".text = ''
        [gdrive]
        type = alias
        remote = /srv/drive
      '';
    };
  };
  scenario = ./Main.btrc;
}
