{ config, inputs, pkgs, lib, ... }:
let
  user = config.settings.users.admin.username;
  icons = import ../../../apps/gaming/icons/package.nix { inherit pkgs inputs; };
  disk = {
    device   = "/dev/nvme0n1";
    swapSize = "20G";  # hibernation image for 16 GB of RAM
  };
  sdCard = {
    label = "SD";  # Jovian automounts it at /run/media/<user>/<label>
  };
  network = {
    lanSubnet = "192.168.50.0/24";
  };
  emulationRoot = "/run/media/${user}/${sdCard.label}/Emulation";
in {
  imports = [ inputs.jovian.nixosModules.default ./session.nix ./touch-keyboard.nix ];
  networking.hostName = "STEAM-DECK-OLED";

  # The Deck's horizontal UEFI console is mode 5. "max" selects the
  # highest-numbered firmware mode, not necessarily the highest resolution.
  boot.loader.systemd-boot.consoleMode = lib.mkForce "5";

  jovian = {
    devices.steamdeck = {
      enable = true;
      autoUpdate = true;  # BIOS + controller firmware; settings.boot.autoEnrollKeys restores keys a BIOS flash clears
    };
    steam = {
      enable = true;
      autoStart = true;
      inherit user;
      desktopSession = "plasma";
    };
    decky-loader.enable = true;
  };
  systemd.tmpfiles.rules = [
    "d /home/${user}/.local 0755 ${user} ${user} -"
    "d /home/${user}/.local/share 0755 ${user} ${user} -"
    "d /home/${user}/.local/share/Steam 0755 ${user} ${user} -"
    "f /home/${user}/.local/share/Steam/.cef-enable-remote-debugging 0644 ${user} ${user} -"  # Decky needs Steam's CEF debugging, which Jovian leaves to the user
  ];

  users.users.${user}.uid = 1000;  # Jovian's SD card automount mounts as UID 1000

  environment.systemPackages = with pkgs; [ google-chrome moonlight-qt chiaki-ng protonup-qt ];

  settings = {
    boot = {
      touchKeyboard = true;
      kernelPackages = pkgs.jovianPackages.linuxPackages;
      plymouth.enable = true;
      autoEnrollKeys = true;
    };
    sleep.hibernateOnSuspend = true;
    disk = {
      inherit (disk) device;
      encryption.enable = true;
      recovery.enable = true;
      immutability.enable = true;
      swap.size = disk.swapSize;
    };
    desktop = {
      defaultSession = "gamescope-wayland";
      outputs = [{ name = "eDP-1"; scaleFactor = 1.35; primary = true; }];
      plasma = {
        virtualKeyboard.enable = true;
        taskbarLaunchers = [
          "applications:org.kde.dolphin.desktop"
          "applications:google-chrome.desktop"
          "applications:org.kde.konsole.desktop"
          "applications:steam.desktop"
          "applications:desktop-keyboard.desktop"
          "applications:return-to-gaming-mode.desktop"
        ];
      };
    };
    networking.lanSubnet = network.lanSubnet;
    nix = {  # the desktop builds the emulators; a handheld should never compile them
      substituters = [ "http://FRACTAL-NORTH.local:8080" ];
      trustedPublicKeys = [
        "nixos-cache-1:UlVz7H6ROdjAipqWb58854E7jNS8j7KpiBLydRjaZmA="
        "semu-cache-1:GAvdDO5fCGcTJsr1yyVLgnVn7TTyFYnfEkb7fd2zUew="
      ];
    };
    users.admin = {
      autoLogin.enable = true;
      autoLock.enable = false;  # every wake already asks for the LUKS passphrase
    };
    apps = {
      nixosctl.caffeine.enable = false;
      steam = {
        protonCustom.enable = false;  # Steam's own Proton under gamescope
        shortcuts = {
          Semu = { exe = "/run/current-system/sw/bin/semu-es-de"; icon = "${icons.semu}"; };
          Moonlight = { exe = "/run/current-system/sw/bin/moonlight"; icon = "${icons.moonlight}"; };
          chiaki-ng = { exe = "/run/current-system/sw/bin/chiaki"; icon = icons.chiaki; };
          Chrome = { exe = "/run/current-system/sw/bin/google-chrome-stable"; icon = icons.chrome; };
        };
      };
      battlenet.enable = true;
      rocksmith.enable = false;
      assettocorsa.enable = false;
      semu = {
        target = "steam-deck";
        paths = {
          emulation_root = emulationRoot;
          roms           = "${emulationRoot}/ES-DE/ES-DE/ROMs";
          media          = "${emulationRoot}/ES-DE/ES-DE/downloaded_media";
          bios           = "${emulationRoot}/RetroArch/config/system";
          mods           = "${emulationRoot}/mods";
          content_root   = "${emulationRoot}/Semu";
          state_root     = "${emulationRoot}/Semu/emulators";
          esde_home      = "${emulationRoot}/ES-DE";
        };
      };
    };
  };
}
