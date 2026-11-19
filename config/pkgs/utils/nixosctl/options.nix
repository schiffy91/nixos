{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.apps.utils.nixosctl.enable = mkSetting bool config.settings.apps.utils.enable;
    settings.apps.utils.nixosctl.configPath = mkSetting str "";
    settings.apps.utils.nixosctl.lab.enable = mkSetting bool false;  # the VM lab, for a machine that runs the graph
    settings.apps.utils.nixosctl.settings = mkSetting (pkgs.formats.toml {}).type {};
    settings.apps.utils.nixosctl.upgradeHooksDir = mkSetting str "nixosctl/upgrade.d";  # under /etc; modules drop pin refreshers here
    settings.apps.utils.nixosctl.pendingHooksDir = mkSetting str "nixosctl/pending.d";  # under /etc; the tool reads this very place
    settings.apps.utils.nixosctl.apps = mkSetting (attrsOf (submodule {
      options.description = mkSetting str "";
      options.commands = mkSetting (attrsOf (submodule {
        options.description = mkSetting str "";
        options.run = lib.mkOption { type = str; };  # executable behind `nixosctl apps <app> <command>`
      })) {};
    })) {};  # modules register their own commands; core only lists and runs them
    # Extra tray entries, inserted above "Open /etc/nixos". Keys are labels, sorted; an
    # attrset with `run` is an entry (kind = "item" | "check" | "radio"; `status` exits 0
    # when checked), any other attrset is a submenu. Modules and hosts merge into it.
    settings.apps.utils.nixosctl.tray.menu = mkSetting (pkgs.formats.json {}).type {};
    # Lines above the entries that say how something stands: the label and the first line `run` prints,
    # read each time the menu opens. While `attention` exits 0 the icon asks for attention. Merged like `menu`.
    settings.apps.utils.nixosctl.tray.status = mkSetting (attrsOf (submodule {
      options.run = lib.mkOption { type = str; };
      options.attention = mkSetting (nullOr str) null;
    })) {};
    settings.apps.utils.nixosctl.caffeine.enable = mkSetting bool config.settings.apps.utils.nixosctl.enable;
  };
}
