{ inputs ? (builtins.getFlake "git+file://${toString ../../..}").inputs
, system ? builtins.currentSystem
}:
# The adapter in config/pkgs/utils/drive: `settings.apps.utils.drive` becomes `services.drive`, and what the
# client must keep is persisted. The units themselves are booted and tested in schiffy91/drive (vm-module).
let
  pkgs = import inputs.nixpkgs { inherit system; };
  inherit (pkgs) lib;
  home = "/fixture/user";
  profile = (inputs.nixpkgs.lib.nixosSystem {
    inherit system;
    specialArgs = { inherit inputs; };
    modules = [
      ../../../config/settings.nix
      ../../../config/pkgs/utils/drive
      ../../../config/system/storage/immutability
      {
        settings = {
          apps.utils.drive = {
            enable = true;
            api = "http://127.0.0.1:18475";
            pinned = [ "Pinned" ];
            placements."Games/Library" = "/sd/drive";
            ignore = [ "*.host-adds-this" ];
            followSeconds = 1;
            quietSeconds = 1;
            readers.deny = [ "never-reads" ];
            tray = true;
            freeFloorBytes = 0;
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
  engine = profile.systemd.services.drive-engine.serviceConfig.ExecStart;
  door = profile.systemd.services.drive-door.serviceConfig.ExecStart;
  holds = message: condition: if condition then true else throw "drive adapter: ${message}";
in assert holds "the engine runs as the admin user, on its store" (profile.systemd.services.drive-engine.serviceConfig.User == "fixture" && lib.hasInfix "--store /var/lib/drive/fixture " engine);
assert holds "a pinned folder, a placement and a host's own ignore reach the engine" (lib.hasInfix "--pin Pinned" engine && lib.hasInfix "--place 'Games/Library=/sd/drive'" engine && lib.hasInfix "--ignore '*.host-adds-this'" engine);
assert holds "the client's own ignores are still there" (lib.hasInfix "--ignore '*.part'" engine);
assert holds "the times and the test address reach the engine" (lib.hasInfix "--quiet 1 --follow 1" engine && lib.hasInfix "--api http://127.0.0.1:18475" engine && lib.hasInfix "--free-floor 0" engine);
assert holds "the door mounts the admin's Drive" (lib.hasInfix "--mount ${home}/Drive" door);
assert holds "the store's directory keeps the group users" (builtins.elem "d /var/lib/drive/fixture 0700 fixture users -" profile.systemd.tmpfiles.rules);
assert holds "the tray follows the setting" (profile.systemd.user.services.drive-tray.wantedBy == [ "graphical-session.target" ]);
assert holds "the client's state is persisted" (lib.all (path: builtins.elem path profile.settings.disk.immutability.persist.paths) [ "/var/lib/drive" "${home}/.config/drive" ]);
pkgs.runCommand "drive-module" { } "touch $out"
