{ lib, ... }: {
  networking = { 
    networkmanager.enable = true;
    firewall = { 
      enable = true;
      allowedTCPPorts = [ 22 80 ]; # SSH and HTTP
    };
  };
  services = {
    avahi = {
      enable = true;
      nssmdns4 = true;
      openFirewall = true;
      publish = {
        enable = true;
        userServices = true;
        addresses = true;
      };
    };
    openssh = {
      enable = true;
      settings = {
        PasswordAuthentication = false;
        AllowUsers = null;
        UseDns = true;
        X11Forwarding = false;
        PermitRootLogin = "prohibit-password";
      };
    };
  };
  programs.ssh = {
    startAgent = true;
    extraConfig = ''
      Host *
          IdentityAgent ~/.1password/agent.sock
          ForwardAgent yes
      '';
  };
  networking.useDHCP = lib.mkDefault true;
}
