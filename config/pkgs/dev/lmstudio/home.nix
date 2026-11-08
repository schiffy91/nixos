{ settings, lib, config, pkgs, ... }:
lib.mkIf (settings.apps.enable && settings.apps.dev.enable && settings.apps.lmstudio.enable && pkgs.stdenv.hostPlatform.isDarwin) {
  # The native macOS app manages its own installation and CLI binaries.
  home.sessionPath = [ "${config.home.homeDirectory}/.lmstudio/bin" ];
}
