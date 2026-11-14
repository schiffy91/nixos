{ lib, ... }:
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
in {
  options = with lib.types; {
    settings.networking.lanSubnet = mkSetting (nullOr str) null; # Fixed networks can opt into subnet-scoped inbound access.
    settings.networking.trustedConnections = mkSetting (listOf str) []; # NetworkManager profile UUIDs; inbound access follows their current subnets.
    settings.networking.ports.udp = mkSetting (listOf int) [];
    settings.networking.ports.tcp = mkSetting (listOf int) [];
    settings.networking.forwardAgentHosts = mkSetting (listOf str) [];
    settings.networking.identityAgent = mkSetting str "~/.1password/agent.sock";
    settings.networking.wifi.backgroundScan.enable = mkSetting bool true; # false: stay on one access point until its signal is lost
    settings.networking.wifi.backgroundScan.signalThreshold = mkSetting int (-65); # dBm; below it, look for a stronger access point
    settings.networking.wifi.backgroundScan.weakSignalInterval = mkSetting ints.positive 30; # seconds between scans below the threshold
    settings.networking.wifi.backgroundScan.recurringInterval = mkSetting (nullOr ints.positive) 300; # seconds between scans above it; null = none
    settings.networking.primaryInterface = mkSetting str "";  # empty = no preference; set to e.g. "eno2" to route outbound via that NIC (prevents asymmetric routing when multiple NICs on one subnet)
  };
}
