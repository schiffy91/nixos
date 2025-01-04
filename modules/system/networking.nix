{ lib, ... }:
let 
  lanSubnet = "10.0.0.0/24";
  sshPort = 22;
  mdnsPort = 5353; # Avahi
  sunshinePort = 47984; # Replace with the actual Sunshine port
in
 {
  # Allow SSH, MDNS, and Sunshine only over LAN
  networking.firewall = {
    enable = true;
    allowedTCPPorts = []; 
    allowedUDPPorts = [];
    extraCommands = ''
      iptables -A nixos-fw -p tcp --dport ${toString sshPort} -s ${lanSubnet} -j nixos-fw-accept
      iptables -A nixos-fw -p udp --dport ${toString mdnsPort} -s ${lanSubnet} -j nixos-fw-accept
      iptables -A nixos-fw -p tcp --dport ${toString sunshinePort} -s ${lanSubnet} -j nixos-fw-accept
    '';
    extraStopCommands = ''
      iptables -D nixos-fw -p tcp --dport ${toString sshPort} -s ${lanSubnet} -j nixos-fw-accept || true
      iptables -D nixos-fw -p udp --dport ${toString mdnsPort} -s ${lanSubnet} -j nixos-fw-accept || true
      iptables -D nixos-fw -p tcp --dport ${toString sunshinePort} -s ${lanSubnet} -j nixos-fw-accept || true
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
          extraConfig = ''
          AllowAgentForwarding yes
        '';
      };
    };
    sunshine = {
      enable = lib.mkDefault false;
      openFirewall = false;
      autoStart = true;
      capSysAdmin = true;
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
