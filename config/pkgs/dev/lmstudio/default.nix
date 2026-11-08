{ config, lib, pkgs, ... }:
let apps = config.settings.apps;
in lib.mkIf (apps.dev.lmstudio.enable && pkgs.stdenv.hostPlatform.isLinux) {
  environment.systemPackages = [ pkgs.lmstudio ];
}
