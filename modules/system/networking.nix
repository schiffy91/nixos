{ config, lib, ... }:
let
  lanSubnet = config.variables.networking.lanSubnet;
  sshPorts = [ 22 ];
  mdnsPorts = [ 5353 ];
  mkLoopbackRule = action: ''iptables -${action} nixos-fw -i lo -j nixos-fw-accept${lib.optionalString (action == "D") " || true"}'';
  mkIptablesRule = { action, proto, port }: ''iptables -${action} nixos-fw -p ${proto} --dport ${toString port} -s ${lanSubnet} -j nixos-fw-accept${lib.optionalString (action == "D") " || true"}'';
  mkPortRules = { action, protos, ports }: lib.concatStringsSep "\n" (lib.lists.flatten (map (proto: map (port: mkIptablesRule { inherit action port proto; }) ports) protos));
  mkServiceRules = action: lib.concatStringsSep "\n" (lib.remove null [
    (lib.optionalString config.variables.networking.loopbackEnabled (mkLoopbackRule action))
    (lib.optionalString config.services.openssh.enable (mkPortRules { inherit action; protos = ["tcp"]; ports = sshPorts; }))
    (lib.optionalString config.services.avahi.enable (mkPortRules { inherit action; protos = ["tcp" "udp"]; ports = mdnsPorts; }))
    (lib.optionalString (lib.lists.length config.variables.networking.ports.tcp != 0) (mkPortRules { inherit action; protos = ["tcp"]; ports = config.variables.networking.ports.tcp; }))
    (lib.optionalString (lib.lists.length config.variables.networking.ports.udp != 0) (mkPortRules { inherit action; protos = ["udp"]; ports = config.variables.networking.ports.udp; }))
  ]);
in
{
  networking.firewall = {
    enable = true;
    allowedTCPPorts = [];
    allowedUDPPorts = [];
    extraCommands = mkServiceRules "A"; # A = Add
    extraStopCommands = mkServiceRules "D"; # D = Drop
  };
  services = {
    avahi = {
      enable = true;
      nssmdns4 = true;
      openFirewall = false;
      publish = {
        enable = true;
        userServices = true;
        addresses = true;
      };
    };
    openssh = {
      enable = true;
      openFirewall = false;
      settings = {
        PasswordAuthentication = false;
        UseDns = true;
        PermitRootLogin = "prohibit-password";
        AllowAgentForwarding = "yes";
      };
    };
    printing.browsed.enable = lib.mkDefault false;
  };

  programs.ssh = {
    startAgent = true;
    extraConfig = ''
      Host *
        IdentityAgent ~/.1password/agent.sock
        ForwardAgent yes
    '';
  };
}
