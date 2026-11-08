{ config, lib, ... }: lib.mkIf (config.settings.apps.dev.git.enable) {
  programs.git.enable = true;
}
