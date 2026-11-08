{ settings, lib, config, pkgs, ... }:
lib.mkIf (settings.apps.dev.lmstudio.enable && pkgs.stdenv.hostPlatform.isDarwin) {
  # The native macOS app manages its own installation and CLI binaries.
  home.sessionPath = [ "${config.home.homeDirectory}/.lmstudio/bin" ];
}
