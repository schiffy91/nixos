{ pkgs, host, ... }: {
  networking = {
    interfaces.${host.network.primaryInterface}.wakeOnLan.enable = true;
    networkmanager.unmanaged = map (mac: "mac:${mac}") host.network.unmanagedMacs;
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
