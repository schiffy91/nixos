{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.utils.nixosctl.enable = mkSetting bool config.settings.apps.utils.enable;
    settings.apps.utils.nixosctl.configPath = mkSetting str "";
    settings.apps.utils.nixosctl.settings = mkSetting (pkgs.formats.toml {}).type {};
    settings.apps.utils.nixosctl.upgradeHooksDir = mkSetting str "nixosctl/upgrade.d";  # under /etc; modules drop pin refreshers here
    settings.apps.utils.nixosctl.caffeine.enable = mkSetting bool config.settings.apps.utils.nixosctl.enable;
  };
}
