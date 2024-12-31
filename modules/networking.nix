{ config, lib, ... }: {
  config = {
    networking = lib.mkDefault { 
      networkmanager.enable = lib.mkDefault true;
      firewall = lib.mkDefault { 
        enable = lib.mkDefault true;
        allowedTCPPorts = lib.mkDefault [ 22 80 ]; # SSH and HTTP
      };
    };
    services = lib.mkDefault {
      avahi = lib.mkDefault {
        enable = lib.mkDefault true;
        nssmdns4 = lib.mkDefault true;
        openFirewall = lib.mkDefault true;
        publish = lib.mkDefault {
          enable = lib.mkDefault true;
          userServices = lib.mkDefault true;
          addresses = lib.mkDefault true;
        };
      };
      openssh = lib.mkDefault {
        settings = lib.mkDefault {
          PasswordAuthentication = lib.mkDefault false;
          AllowUsers = lib.mkDefault null;
          UseDns = lib.mkDefault true;
          X11Forwarding = lib.mkDefault false;
          PermitRootLogin = lib.mkDefault "prohibit-password";
        };
      };
    };
    programs.ssh = lib.mkDefault {
      startAgent = lib.mkDefault true;
      extraConfig = lib.mkDefault
      ''
      Host *
          IdentityAgent ~/.1password/agent.sock
          ForwardAgent yes
      '';
    };
    users.users.${config.variables.user.admin}.openssh.authorizedKeys.keys = lib.mkDefault [
      "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI= ${config.variables.user.admin}"
    ];
  };
}
