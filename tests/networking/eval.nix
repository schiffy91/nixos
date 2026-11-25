{ system ? builtins.currentSystem }:
let
  lock = (builtins.fromJSON (builtins.readFile ../../flake.lock)).nodes.nixpkgs.locked;
  source = builtins.fetchTree { inherit (lock) type owner repo rev narHash; };
  lib = import (source + "/lib");
  evaluate = networking: (import (source + "/nixos/lib/eval-config.nix") {
    inherit system;
    specialArgs.inputs.btrc = let btrc = (builtins.fromJSON (builtins.readFile ../../flake.lock)).nodes.btrc.locked;
      in builtins.getFlake (builtins.flakeRefToString (builtins.removeAttrs btrc [
        "narHash" "lastModified" "lastModifiedDate" "revCount" "dirtyRev" "dirtyShortRev"
      ]));
    modules = [ ../../config/settings.nix ../../config/system/nixos/networking.nix {
      settings.networking = networking;
      networking.hostName = "networking-check";
      boot.isContainer = true;
      system.stateVersion = "24.11";
    } ];
  }).config;
  fixed = evaluate { lanSubnet = "192.168.50.0/24"; };
  roaming = evaluate { trustedConnections = [ "11111111-1111-4111-8111-111111111111" ]; ports.tcp = [ 8080 ]; };
  roamingWifi = evaluate { roaming = true; };
  closed = evaluate {};
  quiet = evaluate { wifi.backgroundScan = { signalThreshold = -76; recurringInterval = null; }; };
in
assert lib.hasInfix "-s 192.168.50.0/24" fixed.networking.firewall.extraCommands;
assert fixed.networking.networkmanager.dispatcherScripts == [];
assert !(fixed.systemd.services ? "nixos-network-trust@");
assert roaming.settings.networking.lanSubnet == null;
assert builtins.length roaming.networking.networkmanager.dispatcherScripts == 3;
assert !(lib.hasInfix "192.168.1.0/24" roaming.networking.firewall.extraCommands);
assert lib.hasInfix "trusted-connections" roaming.networking.firewall.extraCommands;
assert lib.hasInfix "--stop" roaming.networking.firewall.extraStopCommands;
assert roaming.systemd.services ? "nixos-network-trust@";
assert roaming.environment.etc ? "trusted-connections.json";
assert roaming.settings.apps.utils.nixosctl.tray.status ? Network;
assert roaming.settings.apps.utils.nixosctl.tray.menu ? "Trust current network";
assert lib.hasInfix "nixos-network-trust@" roaming.security.polkit.extraConfig;
assert roamingWifi.settings.networking.roaming;
assert builtins.elem "schi-fi" roamingWifi.settings.networking.trustedWiFi;
assert builtins.elem "🐻" roamingWifi.settings.networking.trustedWiFi;
assert builtins.length roamingWifi.networking.networkmanager.dispatcherScripts == 3;
assert roamingWifi.systemd.services ? "nixos-network-trust@";
assert !(lib.hasInfix "-j nixos-fw-accept" closed.networking.firewall.extraCommands);
assert closed.networking.networkmanager.dispatcherScripts == [];
assert !(closed.systemd.services ? "nixos-network-trust@");
assert !(closed.systemd.services ? wifi-background-scan);
assert closed.services.openssh.settings.KbdInteractiveAuthentication == false;
assert builtins.length quiet.networking.networkmanager.dispatcherScripts == 1;
assert quiet.systemd.services ? wifi-background-scan;
{ staticSubnet = true; roamingProfiles = true; roamingWifi = true; unconfiguredNetworksClosed = true; wifiScanOnlyWhenDeclared = true; }
