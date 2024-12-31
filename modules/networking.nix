{ config, ... }: {
  networking = { 
    networkmanager.enable = true;
    firewall = { 
      enable = true;
      allowedTCPPorts = [ 22 80 ]; # SSH and HTTP
    };
  };
  services.openssh = {
    settings = {
      PasswordAuthentication = false;
      AllowUsers = null;
      UseDns = true;
      X11Forwarding = false;
      PermitRootLogin = "prohibit-password";
    };
  };
  programs.ssh = {
  	startAgent = true;
  	extraConfig = 
    ''
	  Host *
	  	  IdentityAgent ~/.1password/agent.sock
	  	  ForwardAgent yes
  	'';
  };
  users.users.${config.userConfig.rootUser}.openssh.authorizedKeys.keys = [
    "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI= ${config.userConfig.rootUser}"
  ];
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
  };
}
