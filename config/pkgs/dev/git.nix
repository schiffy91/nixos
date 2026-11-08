{ config, lib, ... }: lib.mkIf (config.settings.apps.enable && config.settings.apps.dev.enable && config.settings.apps.git.enable) {
  programs.git.enable = true;
}
