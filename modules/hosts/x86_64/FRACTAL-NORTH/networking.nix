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

  # atlantic driver (Aquantia/Marvell AQC) advertises tx-udp-segmentation but
  # mishandles UDP GSO on the wire — kernel itself logs "suspect GRO implementation".
  # Sunshine uses UDP_SEGMENT cmsg per frame; bad hw-segmentation produces malformed
  # packets that downstream gear silently drops. Force software segmentation instead.
  systemd.services.atlantic-tx-udp-seg-off = {
    description = "Disable tx-udp-segmentation on atlantic NIC (driver quirk)";
    wantedBy = [ "multi-user.target" ];
    after = [ "network-pre.target" "sys-subsystem-net-devices-${host.network.primaryInterface}.device" ];
    bindsTo = [ "sys-subsystem-net-devices-${host.network.primaryInterface}.device" ];
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = true;
      ExecStart = "${pkgs.ethtool}/bin/ethtool -K ${host.network.primaryInterface} tx-udp-segmentation off";
    };
  };
}
