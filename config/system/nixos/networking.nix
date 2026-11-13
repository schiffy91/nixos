{ config, lib, pkgs, inputs, ... }:
let
  servicePorts = {
    tcp = lib.unique (lib.optional config.services.openssh.enable 22
      ++ lib.optional config.services.avahi.enable 5353 ++ config.settings.networking.ports.tcp);
    udp = lib.unique (lib.optional config.services.avahi.enable 5353 ++ config.settings.networking.ports.udp);
  };
  roaming = config.settings.networking.trustedConnections != [];
  trustedConfiguration = pkgs.writeText "trusted-connections.json" (builtins.toJSON {
    connections = config.settings.networking.trustedConnections;
    ports = servicePorts;
  });
  trustedConnections = import ./networking/package.nix { inherit pkgs inputs; };
  reconcile = "${trustedConnections}/bin/trusted-connections ${trustedConfiguration}";
  dispatcher = pkgs.writeShellScript "trusted-connections-dispatcher" ''
    case "$2" in
      pre-up|pre-down|down|vpn-pre-down|vpn-down)
        exec ${reconcile} --exclude-interface "$1"
        ;;
      up|vpn-up|dhcp4-change|dhcp6-change|reapply|connectivity-change)
        exec ${reconcile}
        ;;
    esac
  '';
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
  settings.disk.immutability.persist.paths = [
    "/root/.ssh/known_hosts"
    "${config.settings.users.admin.homeDirectory}/.ssh/known_hosts"
  ] ++ lib.optionals config.services.openssh.enable [
    "/etc/ssh"
  ] ++ lib.optionals config.networking.networkmanager.enable [
    "/etc/NetworkManager/"
    "${config.settings.users.admin.homeDirectory}/.cert/nm-openvpn"
  ];
  ##### Firewall Settings #####
  networking = {
    networkmanager = {
      enable = true;
      dispatcherScripts = lib.optionals roaming (map (type: { inherit type; source = dispatcher; }) [ "basic" "pre-up" "pre-down" ]);
    };
    firewall = {
      enable = true;
      allowedTCPPorts = [];
      allowedUDPPorts = [];
      extraCommands = lib.optionalString (config.settings.networking.lanSubnet != null) (mkServiceRules "A")
        + lib.optionalString roaming "\n${reconcile}";
      extraStopCommands = lib.optionalString (config.settings.networking.lanSubnet != null) (mkServiceRules "D")
        + lib.optionalString roaming "\n${reconcile} --stop";
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
    extraConfig = import ../ssh-client.nix { inherit lib; inherit (config.settings.networking) identityAgent forwardAgentHosts; };
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
