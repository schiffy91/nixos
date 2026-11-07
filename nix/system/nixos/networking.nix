{ config, lib, pkgs, ... }:
let
  ##### Firewall Functions #####
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
  networking = {
    networkmanager.enable = true;
    firewall = {
      enable = true;
      allowedTCPPorts = [];
      allowedUDPPorts = [];
      extraCommands = mkServiceRules "A"; # A = Add
      extraStopCommands = mkServiceRules "D"; # D = Drop
    };
  };
  ##### mDNS (e.g. ssh HOSTNAME.local) ##### 
  services = {
    avahi = {
      enable = true;
      nssmdns4 = true;
      openFirewall = false;
      settings.publish = {  # nixpkgs replaced publish.* with avahi-daemon.conf keys
        "disable-publishing" = false;
        disable-user-service-publishing = false;
        publish-addresses = true;
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
      # Remote sessions must use the incoming agent instead of the remote
      # machine's desktop socket. Local sessions keep the 1Password agent.
      # sudo preserves SSH_AUTH_SOCK; root has no desktop agent of its own.
      Match localuser root
        IdentityAgent SSH_AUTH_SOCK
      Match exec "test -n \"$SSH_CONNECTION\" && test -S \"$SSH_AUTH_SOCK\""
        IdentityAgent SSH_AUTH_SOCK
      ${lib.optionalString (config.settings.networking.forwardAgentHosts != []) ''
      Host ${lib.concatStringsSep " " (lib.unique (config.settings.networking.forwardAgentHosts ++ map lib.toLower config.settings.networking.forwardAgentHosts))}
        ForwardAgent yes
      ''}
      Host *
        IdentityAgent ${config.settings.networking.identityAgent}
        ForwardAgent no
    '';
  };
  security.pam.sshAgentAuth.enable = true;
  systemd.services = lib.optionalAttrs (config.settings.networking.primaryInterface != "") {
    disable-non-primary-eth = {
      wantedBy = [ "multi-user.target" ];
      wants = [ "NetworkManager.service" ];
      after = [ "NetworkManager.service" ];
      serviceConfig = { Type = "oneshot"; RemainAfterExit = true; };
      path = [ pkgs.networkmanager ];
      script = ''
        nmcli -t -f DEVICE,TYPE,STATE dev | while IFS=: read -r device type state; do
          case "$state" in
            disconnected|unavailable|unmanaged) continue ;;
          esac
          if [ "$type" = "ethernet" ] && [ "$device" != "${config.settings.networking.primaryInterface}" ]; then
            nmcli device disconnect "$device" || true
          fi
        done
      '';
    };
  };
}
