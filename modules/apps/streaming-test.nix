{ config, pkgs, ... }: {
  environment.systemPackages = with pkgs; [
    moonlight-qt        # Moonlight client for localhost-loopback testing of Sunshine
    tcpdump             # packet inspection during stream attempts
    ethtool             # NIC offload toggling for atlantic driver diagnostics
  ];
  security.sudo.extraRules = [{
    users = [ config.settings.user.admin.username ];
    commands = [
      { command = "/run/current-system/sw/bin/nixos-rebuild"; options = [ "NOPASSWD" ]; }
      { command = "/run/current-system/sw/bin/tcpdump"; options = [ "NOPASSWD" ]; }
      { command = "/run/current-system/sw/bin/ethtool"; options = [ "NOPASSWD" ]; }
    ];
  }];
}
