{ config, inputs, pkgs, lib, host, ... }:
let
  user = config.settings.users.admin.username;
  apps = config.settings.apps;
  steamEnabled = apps.enable && apps.gaming.enable && apps.steam.enable;
in {
  imports = [ inputs.jovian.nixosModules.default ./session.nix ./touch-keyboard.nix ];

  # The Deck's horizontal UEFI console is mode 5. "max" selects the
  # highest-numbered firmware mode, not necessarily the highest resolution.
  boot.loader.systemd-boot.consoleMode = lib.mkForce "5";

  jovian = {
    devices.steamdeck = {
      enable = true;
      autoUpdate = true;  # Run version-gated BIOS/controller update checks; see README before enrolling Secure Boot keys.
    };
    steam = {
      enable = steamEnabled;
      autoStart = steamEnabled;
      inherit user;
      desktopSession = "plasma";
    };
    decky-loader.enable = steamEnabled;
  };
  systemd.tmpfiles.rules = lib.optionals steamEnabled [
    "d /home/${user}/.local 0755 ${user} ${user} -"
    "d /home/${user}/.local/share 0755 ${user} ${user} -"
    "d /home/${user}/.local/share/Steam 0755 ${user} ${user} -"
    "f /home/${user}/.local/share/Steam/.cef-enable-remote-debugging 0644 ${user} ${user} -"  # Decky needs Steam's CEF debugging, which Jovian leaves to the user
  ];

  users.users.${user}.uid = 1000;  # Jovian's SD card automount mounts as UID 1000

  settings = {
    boot = {
      touchKeyboard = true;
      kernelPackages = pkgs.jovianPackages.linuxPackages;
      plymouth.enable = true;
      # Enrollment follows verified external recovery and an independent key backup.
      autoEnrollKeys = false;
    };
    sleep = {
      # The Valve kernel explicitly rejects s2idle on Jupiter/Galileo.
      suspendMode = "deep";
      hibernateOnSuspend = true;
      lockGamingSession = steamEnabled;
    };
    disk = {
      inherit (host.disk) device;
      encryption.enable = true;
      encryption.opportunisticUnlock.enable = true;
      recovery.enable = true;
      immutability.enable = true;
      immutability.persist.paths = lib.optionals config.jovian.decky-loader.enable [ "/var/lib/decky-loader" ];
      swap.size = host.disk.swapSize;
    };
    desktop = {
      defaultSession = if steamEnabled then "gamescope-wayland" else "plasma";
      outputs = [{ name = "eDP-1"; scaleFactor = 1.35; primary = true; }];
      plasma = {
        virtualKeyboard.enable = true;
        taskbarLaunchers = [
          "applications:org.kde.dolphin.desktop"
          "applications:google-chrome.desktop"
          "applications:org.kde.konsole.desktop"
        ] ++ lib.optionals steamEnabled [
          "applications:steam.desktop"
        ] ++ [
          "applications:desktop-keyboard.desktop"
        ] ++ lib.optionals steamEnabled [
          "applications:return-to-gaming-mode.desktop"
        ];
      };
    };
    # Existing home Wi-Fi profile; trusted source subnets follow its DHCP routes.
    networking.trustedConnections = [ "8a75b161-7efd-40a7-9b4c-05f8a57232ff" ];
    networking.forwardAgentHosts = [ "FRACTAL-NORTH" "FRACTAL-NORTH.local" "192.168.50.67" ];
    nix = {  # the desktop builds the emulators; a handheld should never compile them
      substituters = [ "http://FRACTAL-NORTH.local:8080" ];
      trustedPublicKeys = [
        "nixos-cache-1:UlVz7H6ROdjAipqWb58854E7jNS8j7KpiBLydRjaZmA="
        "semu-cache-1:GAvdDO5fCGcTJsr1yyVLgnVn7TTyFYnfEkb7fd2zUew="
      ];
    };
    users.admin = {
      autoLogin.enable = true;
      autoLock.enable = true;
    };
  };
}
