{ config, lib, options, pkgs, inputs, ... }:
let
  servicePorts = {
    tcp = lib.unique (lib.optional config.services.openssh.enable 22
      ++ lib.optional config.services.avahi.enable 5353 ++ config.settings.networking.ports.tcp);
    udp = lib.unique (lib.optional config.services.avahi.enable 5353 ++ config.settings.networking.ports.udp);
  };
  roaming = config.settings.networking.roaming || config.settings.networking.trustedConnections != [];
  trustedConfiguration = pkgs.writeText "trusted-connections.json" (builtins.toJSON {
    connections = config.settings.networking.trustedConnections;
    wifi = config.settings.networking.trustedWiFi;
    ports = servicePorts;
  });
  trustedConnections = import ./networking/package.nix { inherit pkgs inputs; };
  reconcile = "${trustedConnections}/bin/trusted-connections ${trustedConfiguration}";
  trustedConfigPath = "/etc/trusted-connections.json";
  admin = config.settings.users.admin.username;
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
  ##### Wi-Fi Background Scan #####
  # A scan for a stronger access point stalls traffic for seconds. NetworkManager decides
  # when and has no setting for it, so wpa_supplicant is told after each connection.
  backgroundScan = config.settings.networking.wifi.backgroundScan;
  backgroundScanChanged = backgroundScan != lib.mapAttrs (_: option: option.default) options.settings.networking.wifi.backgroundScan;
  backgroundScanPolicy = lib.optionalString backgroundScan.enable (lib.concatMapStringsSep ":" toString [
    "simple" backgroundScan.weakSignalInterval backgroundScan.signalThreshold
    (if backgroundScan.recurringInterval == null then 31536000 else backgroundScan.recurringInterval)  # a year: the policy has no "never"
  ]);
  setBackgroundScan = pkgs.writeShellScript "wifi-background-scan" ''
    supplicant=fi.w1.wpa_supplicant1
    for device in ''${1:-$(ls /sys/class/net)}; do
      [ -e "/sys/class/net/$device/phy80211" ] || continue
      interface=$(${pkgs.systemd}/bin/busctl call $supplicant /fi/w1/wpa_supplicant1 $supplicant GetInterface s "$device" 2>/dev/null | cut -d'"' -f2)
      [ -n "$interface" ] || continue
      network=$(${pkgs.systemd}/bin/busctl get-property $supplicant "$interface" $supplicant.Interface CurrentNetwork | cut -d'"' -f2)
      [ "$network" != / ] || continue
      ${pkgs.systemd}/bin/busctl set-property $supplicant "$network" $supplicant.Network Properties 'a{sv}' 1 bgscan s ${lib.escapeShellArg backgroundScanPolicy}
    done
  '';
  backgroundScanDispatcher = pkgs.writeShellScript "wifi-background-scan-dispatcher" ''
    [ "$2" != up ] || exec ${setBackgroundScan} "$1"
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
  environment.systemPackages = lib.optionals roaming [ trustedConnections ];
  environment.etc = lib.optionalAttrs roaming {
    "trusted-connections.json".source = trustedConfiguration;
  };
  settings.apps.utils.nixosctl.tray = lib.mkIf roaming {
    status.Network.run = "${trustedConnections}/bin/trusted-connections ${trustedConfigPath} --status";
    menu."Trust current network" = {
      kind = "check";
      run = "systemctl start nixos-network-trust@toggle.service";
      status = "${trustedConnections}/bin/trusted-connections ${trustedConfigPath} --is-trusted";
    };
  };
  # The tray toggles trust on the active connection as the admin without a password prompt, only through these units.
  security.polkit.extraConfig = lib.mkIf roaming ''
    polkit.addRule(function(action, subject) {
      if (action.id == "org.freedesktop.systemd1.manage-units" && subject.user == "${admin}"
          && /^nixos-network-trust@(trust|untrust|toggle)\.service$/.test(action.lookup("unit") || "")
          && action.lookup("verb") == "start") {
        return polkit.Result.YES;
      }
    });
  '';
  ##### Firewall Settings #####
  networking = {
    networkmanager = {
      enable = true;
      dispatcherScripts = lib.optionals roaming (map (type: { inherit type; source = dispatcher; }) [ "basic" "pre-up" "pre-down" ])
        ++ lib.optional backgroundScanChanged { type = "basic"; source = backgroundScanDispatcher; };
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
        KbdInteractiveAuthentication = false;
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
  systemd.services = lib.optionalAttrs roaming {
    "nixos-network-trust@" = {
      description = "Update trusted network access (%i)";
      after = [ "NetworkManager.service" "firewall.service" ];
      serviceConfig = {
        Type = "oneshot";
        ExecStart = "${trustedConnections}/bin/trusted-connections ${trustedConfigPath} --%i";
      };
    };
  } // lib.optionalAttrs backgroundScanChanged {
    wifi-background-scan = {  # the connection that is already up; later ones get it from the dispatcher
      wantedBy = [ "multi-user.target" ];
      after = [ "NetworkManager.service" ];
      serviceConfig = { Type = "oneshot"; RemainAfterExit = true; ExecStart = setBackgroundScan; };
    };
  } // lib.optionalAttrs (config.settings.networking.primaryInterface != "") {
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
