{ config, lib, pkgs, ... }:
let apps = config.settings.apps;
in lib.mkIf (apps.enable && apps.dev.enable && apps.lmstudio.enable && pkgs.stdenv.hostPlatform.isLinux) {
  environment.systemPackages = [ pkgs.lmstudio ];
}
