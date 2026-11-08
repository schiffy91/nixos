{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
let
  pkgs = import inputs.nixpkgs { inherit system; };
  inherit (pkgs) lib;
  home = "/fixture/user";
  featurePath = "${home}/.config/rclone";
  profile = enabled: retain: (inputs.nixpkgs.lib.nixosSystem {
    inherit system;
    specialArgs = { inherit inputs; };
    modules = [
      ../../../config/settings.nix
      ../../../config/pkgs/utils/rclone.nix
      ../../../config/system/storage/immutability
      {
        settings = {
          apps.utils.rclone.enable = enabled;
          users.admin = { username = "fixture"; homeDirectory = home; };
          users.agent.enable = false;
          disk = {
            encryption.enable = false;
            immutability = {
              enable = false;
              persist.neededForBoot = [];
              persist.paths = [ "/fixture/shared" ] ++ lib.optional retain featurePath;
            };
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
  profiles = {
    enabled = profile true false;
    disabled = profile false false;
    retained = profile false true;
  };
  runtime = lib.findFirst (package: lib.getName package == "immutability")
    (throw "persistence-only profile did not expose the immutability runtime")
    profiles.enabled.environment.systemPackages;
in (import ../../vm/test.nix { inherit pkgs inputs; }) {
  name = "persistence-feature-disable";
  nodes.machine = { ... }: {
    virtualisation.emptyDiskImages = [ 512 ];
    boot.supportedFilesystems = [ "btrfs" ];
    environment.systemPackages = [ runtime pkgs.btrfs-progs ];
    environment.etc = lib.mapAttrs' (name: config:
      lib.nameValuePair "persistence-features/${name}.toml" {
        source = config.environment.etc."immutability/spec.toml".source;
      }) profiles // {
      "persistence-features/profiles.json".text = builtins.toJSON
        (lib.mapAttrs (_: config: {
          enabled = config.settings.apps.utils.rclone.enable;
          paths = config.settings.disk.immutability.persist.paths;
        }) profiles);
    };
  };
  scenario = ./Main.btrc;
}
