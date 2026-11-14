{ pkgs, host, ... }: {
  networking = {
    interfaces.${host.network.primaryInterface}.wakeOnLan.enable = true;
    networkmanager.unmanaged = map (mac: "mac:${mac}") host.network.unmanagedMacs;
  };
  settings.networking = {
    forwardAgentHosts = [ "STEAM-DECK-OLED" "STEAM-DECK-OLED.local" "192.168.50.52" ];
    lanSubnet = host.network.lanSubnet;
    primaryInterface = host.network.primaryInterface;
  };
  services = {
    ##### Thunderbolt #####
    hardware.bolt.enable = true;
  };

  # atlantic driver (Aquantia/Marvell AQC) advertises tx-udp-segmentation but
  # mishandles UDP GSO on the wire — kernel itself logs "suspect GRO implementation".
  # Sunshine uses UDP_SEGMENT cmsg per frame; bad hw-segmentation produces malformed
  # packets that downstream gear silently drops. Force software segmentation instead.
  systemd.services.atlantic-tx-udp-seg-off = {
    description = "Disable tx-udp-segmentation on atlantic NIC (driver quirk)";
    wantedBy = [ "sys-subsystem-net-devices-${host.network.primaryInterface}.device" ];  # when the NIC appears, not at boot: a boot without it (a VM of this host) waited 90 s for it and ended degraded
    after = [ "network-pre.target" "sys-subsystem-net-devices-${host.network.primaryInterface}.device" ];
    bindsTo = [ "sys-subsystem-net-devices-${host.network.primaryInterface}.device" ];
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = true;
      ExecStart = "${pkgs.ethtool}/bin/ethtool -K ${host.network.primaryInterface} tx-udp-segmentation off";
    };
  };
}
