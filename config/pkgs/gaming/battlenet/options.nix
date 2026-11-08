{ lib, config, ... }:
{
  options.settings.apps.gaming.battlenet = {
    enable = lib.mkOption {
      type = lib.types.bool;
      default = config.settings.apps.gaming.steam.protonCustom.enable;
    };
    prefix = lib.mkOption {
      type = lib.types.str;
      default = "${config.settings.users.admin.homeDirectory}/Games/Battle.net/prefix";
      description = "Wine prefix shared by Battle.net and its games.";
    };
  };
}
