{ config, inputs, pkgs, ... }:
let
  user = config.settings.users.admin.username;
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
  imports = [ inputs.jovian.nixosModules.default ];
  networking.hostName = "STEAM-DECK-OLED";

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
  systemd.tmpfiles.rules = [ "f /home/${user}/.local/share/Steam/.cef-enable-remote-debugging 0644 ${user} ${user} -" ];  # Decky needs Steam's CEF debugging, which Jovian leaves to the user

  users.users.${user}.uid = 1000;  # Jovian's SD card automount mounts as UID 1000

  environment.systemPackages = with pkgs; [ moonlight-qt chiaki-ng firefox protonup-qt ];

  ##### Touchscreen LUKS passphrase #####
  boot.initrd.unl0kr = {
    enable = true;
    settings.general.backend = "drm";
  };

  settings = {
    boot = {
      kernelPackages = pkgs.jovianPackages.linuxPackages;
      plymouth.enable = false;  # unl0kr owns the screen in initrd
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
    desktop.defaultSession = "gamescope-wayland";
    networking.lanSubnet = network.lanSubnet;
    users.admin = {
      autoLogin.enable = true;
      autoLock.enable = false;  # every wake already asks for the LUKS passphrase
    };
    apps = {
      nixosctl.caffeine.enable = false;
      steam.protonCustom.enable = false;  # Steam's own Proton under gamescope
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
