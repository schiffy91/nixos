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
    settings.networking.primaryInterface = mkSetting str "";  # empty = no preference; set to e.g. "eno2" to route outbound via that NIC (prevents asymmetric routing when multiple NICs on one subnet)
  };
}
