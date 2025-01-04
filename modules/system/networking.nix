{ config, lib, ... }:
let 
  lanSubnet = "10.0.0.0/24";
  sshPort = 22;
  mdnsPort = 5353; # Avahi
  sunshineTcpPorts = [ 47984 47989 47990 48010 ];
  sunshineUdpPortRanges = [
    { from = 47998; to = 48000; }
    { from = 8000;  to = 8010; }
  ];
in
{
  networking.firewall = {
    enable = true;
    allowedTCPPorts = [];
    allowedUDPPorts = [];
    extraCommands = ''
      iptables -A nixos-fw -p tcp --dport ${toString sshPort} -s ${lanSubnet} -j nixos-fw-accept
      iptables -A nixos-fw -p udp --dport ${toString mdnsPort} -s ${lanSubnet} -j nixos-fw-accept
    '' 
    + lib.mkIf config.services.sunshine.enabled ''
      ${lib.concatStringsSep "\n" (map (port: ''iptables -A nixos-fw -p tcp --dport ${toString port} -s ${lanSubnet} -j nixos-fw-accept'') sunshineTcpPorts)}
      ${lib.concatStringsSep "\n" (map (range: ''iptables -A nixos-fw -p udp --dport ${toString range.from}:${toString range.to} -s ${lanSubnet} -j nixos-fw-accept'') sunshineUdpPortRanges)}
    '';
    extraStopCommands = ''
      # SSH + mDNS
      iptables -D nixos-fw -p tcp --dport ${toString sshPort} -s ${lanSubnet} -j nixos-fw-accept || true
      iptables -D nixos-fw -p udp --dport ${toString mdnsPort} -s ${lanSubnet} -j nixos-fw-accept || true
    ''
    + lib.mkIf config.services.sunshine.enabled ''
      ${lib.concatStringsSep "\n" (map (port: ''iptables -D nixos-fw -p tcp --dport ${toString port} -s ${lanSubnet} -j nixos-fw-accept || true'') sunshineTcpPorts)}
      ${lib.concatStringsSep "\n" (map (range: ''iptables -D nixos-fw -p udp --dport ${toString range.from}:${toString range.to} -s ${lanSubnet} -j nixos-fw-accept || true'') sunshineUdpPortRanges)}
    '';
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
