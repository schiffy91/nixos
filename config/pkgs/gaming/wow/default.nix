{ config, pkgs, lib, inputs, ... }:
let
  tools = import ./package.nix { inherit pkgs inputs settings; };
  addons = import ./bundle.nix { inherit pkgs; inherit (config.settings.apps.gaming.wow) addons; };
  settings = pkgs.writeText "wow-settings.json" (builtins.toJSON {
    inherit (config.settings.apps.gaming.wow) directory;
    inherit addons;
    settings = lib.optionalString (builtins.pathExists ./settings) "${./settings}";  # absent until the first capture
    excluded = config.settings.apps.gaming.wow.settingsExcluded;
  });
in lib.mkIf config.settings.apps.gaming.wow.enable {
  environment.systemPackages = [ tools ];
  environment.etc."${config.settings.apps.utils.nixosctl.upgradeHooksDir}/wow-addons" = lib.mkIf config.settings.apps.gaming.wow.upgradeAddons {
    source = "${tools}/bin/wow-addons-lock";
  };
  settings.apps.utils.nixosctl.apps.wow = {
    description = "World of Warcraft addons and settings";
    commands = {
      addons-install = { description = "Link the configured addons into the game now."; run = "${tools}/bin/wow-addons-install"; };
      addons-refresh = { description = "Refresh the addon release pins in the repository."; run = "${tools}/bin/wow-addons-lock"; };
      settings-apply = { description = "Write the committed layout and addon settings into the game."; run = "${pkgs.writeShellScript "wow-settings-apply" ''exec ${tools}/bin/wow-settings apply''}"; };
      settings-capture = { description = "Copy the game's layout and addon settings into the repository."; run = "${pkgs.writeShellScript "wow-settings-capture" ''exec ${tools}/bin/wow-settings capture''}"; };
    };
  };

  systemd.user.services.wow-addons = {
    description = "Install configured World of Warcraft addons";
    wantedBy = [ "default.target" ];
    unitConfig.ConditionUser = config.settings.users.admin.username;
    serviceConfig = {
      Type = "oneshot";
      ExecStart = [ "${tools}/bin/wow-addons-install" "${tools}/bin/wow-settings apply" ];
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
