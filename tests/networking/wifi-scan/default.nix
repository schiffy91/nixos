{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree { inherit (locked) type owner repo rev narHash; }) { inherit system; };
  btrc = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.btrc.locked;
  inputs.btrc = builtins.getFlake (builtins.flakeRefToString (builtins.removeAttrs btrc [
    "narHash" "lastModified" "lastModifiedDate" "revCount" "dirtyRev" "dirtyShortRev"
  ]));
  # The policy wpa_supplicant holds for the network wlan1 is connected to.
  policy = pkgs.writeShellApplication {
    name = "wifi-scan-policy";
    runtimeInputs = [ pkgs.systemd pkgs.jq ];
    text = ''
      supplicant=fi.w1.wpa_supplicant1
      interface=$(busctl call $supplicant /fi/w1/wpa_supplicant1 $supplicant GetInterface s wlan1 | cut -d'"' -f2)
      network=$(busctl get-property $supplicant "$interface" $supplicant.Interface CurrentNetwork | cut -d'"' -f2)
      if [ "''${1:-}" = set ]; then
        busctl set-property $supplicant "$network" $supplicant.Network Properties 'a{sv}' 1 bgscan s "$2"
      else
        busctl --json=short get-property $supplicant "$network" $supplicant.Network Properties | jq -r .data.bgscan.data
      fi
    '';
  };
  # Each machine is a home network and a device on it: two simulated radios,
  # the first an access point with two addresses under one name.
  machine = backgroundScan: { lib, ... }: {
    imports = [ ../../../config/settings.nix ../../../config/system/nixos/networking.nix ];
    _module.args.inputs = inputs;
    settings.networking.wifi.backgroundScan = backgroundScan;
    boot.kernelModules = [ "mac80211_hwsim" ];
    services.hostapd = {
      enable = true;
      radios.wlan0 = {
        band = "2g";
        channel = 6;
        countryCode = "US";
        networks.wlan0 = { ssid = "home"; bssid = "02:00:00:00:00:00"; authentication.mode = "none"; };
        networks.wlan0-1 = { ssid = "home"; bssid = "02:00:00:00:00:01"; authentication.mode = "none"; };
      };
    };
    networking.wireless.enable = lib.mkOverride 5 true;  # test VMs switch wpa_supplicant off
    networking.networkmanager.unmanaged = [ "interface-name:wlan0" "interface-name:wlan0-1" ];
    environment.systemPackages = [ policy ];
  };
in (import ../../lib/vm-test.nix { inherit pkgs inputs; }) {
  name = "wifi-background-scan";
  nodes.stock = machine { };
  nodes.tuned = machine { signalThreshold = -76; recurringInterval = null; };
  nodes.off = machine { enable = false; };
  scenario = ./Main.btrc;
}
