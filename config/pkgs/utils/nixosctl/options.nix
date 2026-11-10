{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.utils.nixosctl.enable = mkSetting bool config.settings.apps.utils.enable;
    settings.apps.utils.nixosctl.configPath = mkSetting str "";
    settings.apps.utils.nixosctl.settings = mkSetting (pkgs.formats.toml {}).type {};
    settings.apps.utils.nixosctl.upgradeHooksDir = mkSetting str "nixosctl/upgrade.d";  # under /etc; modules drop pin refreshers here
    settings.apps.utils.nixosctl.apps = mkSetting (attrsOf (submodule {
      options.description = mkSetting str "";
      options.commands = mkSetting (attrsOf (submodule {
        options.description = mkSetting str "";
        options.run = lib.mkOption { type = str; };  # executable behind `nixosctl apps <app> <command>`
      })) {};
    })) {};  # modules register their own commands; core only lists and runs them
    settings.apps.utils.nixosctl.caffeine.enable = mkSetting bool config.settings.apps.utils.nixosctl.enable;
  };
}
