{ pkgs, inputs }:
let
  lib = pkgs.lib;
  evaluate = settings: (inputs.nixpkgs.lib.nixosSystem {
    system = pkgs.stdenv.hostPlatform.system;
    specialArgs = { inherit inputs; };
    modules = [
      ../../../config/settings.nix
      ../../../config/pkgs/gaming/wow
      {
        nixpkgs.pkgs = pkgs;
        system.stateVersion = "24.11";
        settings.users.admin.homeDirectory = "/srv/player home";
        settings.apps.gaming.wow.enable = true;
      }
      { inherit settings; }
    ];
  }).config;
  defaults = evaluate {};
  overridden = evaluate {
    apps.gaming.battlenet.prefix = "/external/games/Battle.net";
    apps.gaming.wow.addons = lib.mkForce [];
  };
  otherClient = evaluate {
    apps.gaming.wow.directory = "/external/WoW/_retail_";
    apps.gaming.wow.enable = lib.mkForce false;
  };
in
assert defaults.settings.apps.gaming.battlenet.prefix == "/srv/player home/Games/Battle.net/prefix";
assert defaults.settings.apps.gaming.wow.directory == "/srv/player home/Games/Battle.net/prefix/drive_c/Program Files (x86)/World of Warcraft/_forever_";
assert builtins.length defaults.settings.apps.gaming.wow.addons == builtins.length (builtins.attrNames (builtins.fromJSON (builtins.readFile ../../../config/pkgs/gaming/wow/addons.lock.json)));
assert overridden.settings.apps.gaming.wow.directory == "/external/games/Battle.net/drive_c/Program Files (x86)/World of Warcraft/_forever_";
assert overridden.settings.apps.gaming.wow.addons == [];
assert !(defaults.settings.apps.gaming.wow ? detailsProfile);
assert defaults.systemd.user.services.wow-addons.unitConfig.ConditionUser == defaults.settings.users.admin.username;
assert defaults.systemd.user.timers.wow-addons.timerConfig.OnUnitInactiveSec == "1min";
assert !(defaults.system.activationScripts ? wowAddons);
assert otherClient.settings.apps.gaming.wow.directory == "/external/WoW/_retail_";
assert !(otherClient.systemd.user.services ? wow-addons);
assert !(otherClient.systemd.user.timers ? wow-addons);
true
