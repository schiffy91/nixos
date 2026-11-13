{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  imports = [ ./agents/options.nix ];
  options = with lib.types; {
    settings.apps.dev.enable = mkSetting bool config.settings.apps.enable;
    settings.apps.dev.packages = mkSetting (listOf package) [];
    settings.apps.dev.distrobox.enable = mkSetting bool false;
    settings.apps.dev.bash.enable = mkSetting bool config.settings.apps.dev.enable;
    settings.apps.dev.claude.enable = mkSetting bool config.settings.apps.dev.enable;
    settings.apps.dev.codex.enable = mkSetting bool config.settings.apps.dev.enable;
    settings.apps.dev.git.enable = mkSetting bool config.settings.apps.dev.enable;
    settings.apps.dev.git.includes = mkSetting (listOf str) [];
    settings.apps.dev.antigravity.enable = mkSetting bool false;
    settings.apps.dev.lmstudio.enable = mkSetting bool false;
    settings.apps.dev.shell.aliases = mkSetting (attrsOf str) {}; # host extras; shared aliases live in shell.nix
    settings.apps.dev.zsh.enable = mkSetting bool (config.settings.apps.dev.enable && config.settings.desktop.enable);
    settings.apps.dev.nixd.enable = mkSetting bool (config.settings.apps.dev.enable && config.settings.desktop.enable);
    settings.apps.dev.vscode.enable = mkSetting bool config.settings.apps.dev.enable;
  };
}
