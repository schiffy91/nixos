{ config, lib, ... }:
let apps = config.settings.apps;
in lib.mkIf (apps.enable && apps.utils.enable && apps.mullvad.enable) {
  settings.disk.immutability.persist.paths = [
    "/etc/mullvad-vpn/"
    "${config.settings.users.admin.homeDirectory}/.config/Mullvad VPN"
  ];
  services.mullvad-vpn = {
    enable = true;
    gui.enable = true;
  };
  systemd.services.mullvad-daemon = {
    wants = lib.mkForce [ "network.target" ];
    after = lib.mkForce [ "NetworkManager.service" "systemd-resolved.service" ];
  };
}
