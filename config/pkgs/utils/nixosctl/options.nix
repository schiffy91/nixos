{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.nixosctl.enable = mkSetting bool true;
    settings.apps.nixosctl.configPath = mkSetting str "";
    settings.apps.nixosctl.settings = mkSetting (pkgs.formats.toml {}).type {};
    settings.apps.nixosctl.upgradeHooksDir = mkSetting str "nixosctl/upgrade.d";  # under /etc; modules drop pin refreshers here
    settings.apps.nixosctl.caffeine.enable = mkSetting bool true;
  };
}
