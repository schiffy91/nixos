{ config, lib, pkgs, ... }:
lib.mkIf (config.settings.apps.enable && config.settings.apps.utils.enable && config.settings.apps.cider.enable) {
  settings.disk.immutability.persist.paths = [ "${config.settings.users.admin.homeDirectory}/.config/sh.cider.genten" ];
  environment.systemPackages = [
    (pkgs.cider-2.overrideAttrs (old: {
      postFixup = (old.postFixup or "") + ''
        substituteInPlace $out/share/applications/cider-2.desktop \
          --replace-fail "Exec=cider-2" "Exec=cider-2 --force-device-scale-factor=1 --ozone-platform=wayland --enable-features=WaylandWindowDecorations"
      '';
    }))
  ];
}
