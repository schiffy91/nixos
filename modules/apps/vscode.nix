{ config, lib, pkgs-unstable, ... }: lib.mkIf (config.settings.apps.enable && config.settings.apps.vscode.enable) {
  environment.systemPackages = [ pkgs-unstable.vscode ];
}
