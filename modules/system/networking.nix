{ config, lib, ... }:
let
  ##### Firewall Functions ##### 
  #TODO networking.nftables.ruleset is probably the more kosher way to do this. https://discourse.nixos.org/t/open-firewall-ports-only-towards-local-network/13037
  mkIptablesRule = { action, proto, port }: ''iptables -${action} nixos-fw -p ${proto} --dport ${toString port} -s ${config.settings.networking.lanSubnet} -j nixos-fw-accept${lib.optionalString (action == "D") " || true"}'';
  mkPortRules = { action, protos, ports }: lib.concatStringsSep "\n" (lib.lists.flatten (map (proto: map (port: mkIptablesRule { inherit action port proto; }) ports) protos));
  mkServiceRules = action: lib.concatStringsSep "\n" (lib.remove null [
    (lib.optionalString config.services.openssh.enable (mkPortRules { inherit action; protos = ["tcp"]; ports = [ 22 ]; })) # SSH
    (lib.optionalString config.services.avahi.enable (mkPortRules { inherit action; protos = ["tcp" "udp"]; ports = [ 5353 ]; })) # Avahi
    (lib.optionalString (lib.lists.length config.settings.networking.ports.tcp != 0) (mkPortRules { inherit action; protos = ["tcp"]; ports = config.settings.networking.ports.tcp; }))
    (lib.optionalString (lib.lists.length config.settings.networking.ports.udp != 0) (mkPortRules { inherit action; protos = ["udp"]; ports = config.settings.networking.ports.udp; }))
  ]);
in {
  ##### Firewall Settings ##### 
  networking.firewall = {
    enable = true;
    allowedTCPPorts = [];
    allowedUDPPorts = [];
    extraCommands = mkServiceRules "A"; # A = Add
    extraStopCommands = mkServiceRules "D"; # D = Drop
  };
  ##### mDNS (e.g. ssh HOSTNAME.local) ##### 
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
    ##### SSH Server ##### 
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
  ##### SSH Client ##### 
  programs.ssh = {
    startAgent = true;
    extraConfig = ''
      Host *
        IdentityAgent ${config.settings.networking.identityAgent}
        ForwardAgent yes
    '';
  };
  security.pam.sshAgentAuth.enable = true;
  ##### INITRD SSH ##### 
  #TODO: Fix this – currently broken (see links below)
  boot.initrd.network = {
    enable = true;
    ssh = {
      enable = true;
      authorizedKeys = [ "${config.settings.user.admin.authorizedKey} ${config.settings.user.admin.username}"];
      # https://search.nixos.org/options?channel=24.11&show=boot.initrd.network.ssh.hostKeys&from=0&size=50&sort=relevance&type=packages&query=boot.initrd.network.ssh
      # https://search.nixos.org/options?channel=24.11&show=boot.initrd.secrets&from=0&size=50&sort=relevance&type=packages&query=initrd
      hostKeys = [ 
        "${config.settings.secrets.path}/${config.settings.secrets.initrd.rsaKeyFile}"
        "${config.settings.secrets.path}/${config.settings.secrets.initrd.ed25519KeyFile}"
      ];
    };
  };
}
