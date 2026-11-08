{ lib, config, pkgs, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  imports = [ ./agents/options.nix ];
  options = with lib.types; {
    settings.apps.dev.enable = mkSetting bool true;
    settings.apps.dev.packages = mkSetting (listOf package) [];
    settings.apps.distrobox.enable = mkSetting bool false;
    settings.apps.bash.enable = mkSetting bool true;
    settings.apps.claude.enable = mkSetting bool true;
    settings.apps.codex.enable = mkSetting bool true;
    settings.apps.git.enable = mkSetting bool true;
    settings.apps.git.includes = mkSetting (listOf str) [];
    settings.apps.gemini.enable = mkSetting bool false;
    settings.apps.lmstudio.enable = mkSetting bool false;
    settings.apps.shell.aliases = mkSetting (attrsOf str) {}; # host extras; shared aliases live in shell.nix
    settings.apps.zsh.enable = mkSetting bool false;
    settings.apps.vscode.enable = mkSetting bool true;
  };
}
