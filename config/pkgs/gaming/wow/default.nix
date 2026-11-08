{ config, pkgs, lib, inputs, ... }:
let
  tools = import ./package.nix { inherit pkgs inputs settings; };
  addons = import ./bundle.nix { inherit pkgs; inherit (config.settings.apps.gaming.wow) addons; };
  settings = pkgs.writeText "wow-settings.json" (builtins.toJSON {
    inherit (config.settings.apps.gaming.wow) directory;
    inherit addons;
  });
in lib.mkIf config.settings.apps.gaming.wow.enable {
  environment.systemPackages = [ tools ];
  environment.etc."${config.settings.apps.utils.nixosctl.upgradeHooksDir}/wow-addons".source = "${tools}/bin/wow-addons-lock";

  systemd.user.services.wow-addons = {
    description = "Install configured World of Warcraft addons";
    wantedBy = [ "default.target" ];
    unitConfig.ConditionUser = config.settings.users.admin.username;
    serviceConfig = {
      Type = "oneshot";
      ExecStart = "${tools}/bin/wow-addons-install";
      UMask = "0022";
    };
  };
  # Battle.net can install the game after system activation.
  systemd.user.timers.wow-addons = {
    wantedBy = [ "timers.target" ];
    unitConfig.ConditionUser = config.settings.users.admin.username;
    timerConfig = {
      OnStartupSec = "30s";
      OnUnitInactiveSec = "1min";
    };
  };
}
