{ pkgs, host, ... }: {
  ##### Networking #####
  networking = {
    interfaces.${host.network.primaryInterface}.wakeOnLan.enable = true;
    networkmanager.unmanaged = [ "mac:24:f5:a2:f1:4d:9b" ];  # Linksys USB3GIGV1 dongle breaks WoL routing
  };
  settings.networking = {
    lanSubnet = host.network.lanSubnet;
    primaryInterface = host.network.primaryInterface;
  };
  programs.openvpn3.enable = true;
  services.resolved.enable = true;
  services.mullvad-vpn = {
    enable = true;
    package = pkgs.mullvad-vpn;
  };
  ##### Thunderbolt #####
  services.hardware.bolt.enable = true;
}
