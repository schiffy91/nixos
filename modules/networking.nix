{ config, lib, ... }: {
  networking = lib.mkDefault { 
    networkmanager.enable = lib.mkDefault true;
    firewall = lib.mkDefault { 
      enable = lib.mkDefault true;
      allowedTCPPorts = lib.mkDefault [ 22 80 ]; # SSH and HTTP
    };
  };
  services = lib.mkDefault {
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
      settings = {
        PasswordAuthentication = false;
        AllowUsers = null;
        UseDns = true;
        X11Forwarding = false;
        PermitRootLogin = "prohibit-password";
      };
    };
  };
  programs.ssh = lib.mkDefault {
    startAgent = true;
    extraConfig = ''
      Host *
          IdentityAgent ~/.1password/agent.sock
          ForwardAgent yes
      '';
  };
  users.users.${config.variables.user.admin}.openssh.authorizedKeys.keys = [
    "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI= ${config.variables.user.admin}"
  ];
}
