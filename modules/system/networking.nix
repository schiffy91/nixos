{ config, lib, ... }:
let
  lanSubnet = config.variables.networking.lanSubnet;
  sshPorts = [ 22 ];
  mdnsPorts = [ 5353 ];
  sunshineTcpPorts = [ 47984 47989 47990 48010 ];
  sunshineUdpPorts = lib.lists.flatten (map (range: lib.lists.range range.from range.to)[ { from = 47998; to = 48000; } { from = 8000; to = 8010; }]);
  mkIptablesRule = { action, proto, port }: "iptables -${action} nixos-fw -p ${proto} --dport ${toString port} -s ${lanSubnet} -j nixos-fw-accept${lib.optionalString (action == "D") " || true"}";
  mkPortRules = { action, proto, ports }: lib.concatStringsSep "\n" (map (port: mkIptablesRule { inherit action proto port; }) ports);
  mkServiceRules = action:
    lib.concatStringsSep "\n" (lib.remove null [
      (lib.optionalString config.services.openssh.enable (mkPortRules { inherit action; proto = "tcp"; ports = sshPorts; }))
      (lib.optionalString config.services.avahi.enable (mkPortRules { inherit action; proto = "udp"; ports = mdnsPorts; }))
      (lib.optionalString config.services.sunshine.enabled (mkPortRules { inherit action; proto = "tcp"; ports = sunshineTcpPorts; }))
      (lib.optionalString config.services.sunshine.enabled (mkPortRules { inherit action; proto = "udp"; ports = sunshineUdpPorts; }))
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
