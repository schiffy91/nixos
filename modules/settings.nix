{ lib, config, ... }: 
let mkSetting = type: defaultValue: lib.mkOption { type = type; default = defaultValue; }; 
in {
  options = with lib.types; {
    ##### Secrets ##### 
    settings.secrets.path = mkSetting str "/etc/nixos/secrets";
    settings.secrets.hashedPasswordFile = mkSetting str "hashed_password.txt";
    ##### Admin ##### 
    settings.user.admin.username = mkSetting str "alexanderschiffhauer";
    settings.user.admin.publicName = mkSetting str "Alexander Schiffhauer";
    settings.user.admin.publicEmail = mkSetting str "Alexander.Schiffhauer@gmail.com";
    settings.user.admin.autoLogin.enable = mkSetting bool false;
    settings.user.admin.autoLock.enable = mkSetting bool true;
    settings.user.admin.autoUnlockWallet.enabled = mkSetting bool true;
    settings.user.admin.authorizedKey = mkSetting str "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIAOOxJXmhrSalqwuZKRncqzBgSuWDXiOhvSlS8pLDeFI";
    settings.user.admin.homeManager.enable = mkSetting bool true;
    ##### Disk ##### 
    settings.disk.device = mkSetting str "";
    ##### Disk: Labels #####
    settings.disk.boot.efiSysMountPoint = mkSetting str "/boot";
    settings.disk.label.disk = mkSetting str "disk";
    settings.disk.label.main = mkSetting str "main";
    settings.disk.label.boot = mkSetting str "boot";
    settings.disk.label.root = mkSetting str "root";
    settings.disk.partlabel.boot = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.boot}";
    settings.disk.partlabel.root = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.root}";
    settings.disk.by.partlabel.boot = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.boot}";
    settings.disk.by.partlabel.root = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.root}";
    settings.disk.by.mapper.root = mkSetting str "/dev/mapper/${config.settings.disk.label.root}";
    ##### Disk: Subvolumes #####
    settings.disk.subvolumes.volumes = mkSetting (listOf (submodule{ 
      options = { 
        name = mkSetting str null; 
        mountPoint = mkSetting str null; 
        mountOptions = mkSetting (listOf str) [ "compress=zstd" "noatime" ]; 
        neededForBoot = mkSetting bool true;
        resetOnBoot =  mkSetting bool false;
        flag = mkSetting (enum [ "none" "swap" "snapshots" "root"]) "none";
      };
    }))
    [
      { name = "@root"; mountPoint = "/"; flag = "root"; resetOnBoot = true; }
      { name = "@home"; mountPoint = "/home"; resetOnBoot = true; }
      { name = "@nix"; mountPoint = "/nix"; }
      { name = "@var"; mountPoint = "/var"; }
      { name = "@snapshots"; mountPoint = "/.snapshots"; flag = "snapshots"; }
      { name = "@swap"; mountPoint = "/.swap"; mountOptions = []; flag = "swap"; neededForBoot = false; }
    ];
    settings.disk.subvolumes.snapshots.name = mkSetting str (toString ((lib.lists.findFirst (volume: volume.flag == "snapshots") null config.settings.disk.subvolumes.volumes).name));
    settings.disk.subvolumes.snapshots.mountPoint = mkSetting str (toString ((lib.lists.findFirst (volume: volume.flag == "snapshots") null config.settings.disk.subvolumes.volumes).mountPoint));
    settings.disk.subvolumes.names.resetOnBoot = mkSetting str (lib.concatMapStringsSep " " (volume: volume.name) (lib.filter (volume: volume.resetOnBoot) config.settings.disk.subvolumes.volumes));
    settings.disk.subvolumes.nameMountPointPairs.resetOnBoot = mkSetting str (lib.concatMapStringsSep " " (volume: "${volume.name}=${volume.mountPoint}") (lib.filter (volume: volume.resetOnBoot) config.settings.disk.subvolumes.volumes));
    ##### Disk: Swap #####
    settings.disk.swap.enable = mkSetting bool true;
    settings.disk.swap.size = mkSetting str "";
    ##### Disk: Encryption #####
    settings.disk.encryption.enable = mkSetting bool true;
    settings.disk.encryption.plainTextPasswordFile = mkSetting str "/tmp/plain_text_password.txt";
    ##### Disk: Immutability #####
    settings.disk.immutability.enable = mkSetting bool false;
    settings.disk.immutability.persist.snapshots.cleanName = mkSetting str "CLEAN";
    settings.disk.immutability.persist.paths = mkSetting (listOf str) [
      ##### Core System Files #####
      "/etc/machine-id"
      "/etc/nixos"
      "/etc/ssh"
      "/etc/NetworkManager/"
      "/usr/bin/env"
      "/var/lib/bluetooth"
      "/var/lib/nixos"
      "/var/lib/systemd/coredump"
      "/var/log"
      "/root/.cache/nix/"
      ##### Secure Boot #####
      "${config.settings.boot.pkiBundle}"
      ##### Files & Folders #####
      "/home/${config.settings.user.admin.username}/Downloads"
      "/home/${config.settings.user.admin.username}/.bash_history"
      "/home/${config.settings.user.admin.username}/.config/dconf/user"
      "/home/${config.settings.user.admin.username}/.config/xsettingsd/xsettingsd.conf"
      "/home/${config.settings.user.admin.username}/.cache"
      "/home/${config.settings.user.admin.username}/.pki"
      ##### SSH #####
      "/home/${config.settings.user.admin.username}/.ssh/known_hosts"
      ##### Network Manager #####
      "/home/${config.settings.user.admin.username}/.cert/nm-openvpn"
      ##### Direnv #####
      "/home/${config.settings.user.admin.username}/.local/share/direnv"
      ##### Plasma #####
      "/home/${config.settings.user.admin.username}/.config/gtk-3.0"
      "/home/${config.settings.user.admin.username}/.config/gtk-4.0"
      "/home/${config.settings.user.admin.username}/.config/gtkrc-2.0"
      "/home/${config.settings.user.admin.username}/.config/gtkrc"
      "/home/${config.settings.user.admin.username}/.config/kcmfonts"
      "/home/${config.settings.user.admin.username}/.config/kcminputrc"
      "/home/${config.settings.user.admin.username}/.config/kdedefaults"
      "/home/${config.settings.user.admin.username}/.config/kdeglobals"
      "/home/${config.settings.user.admin.username}/.config/konsolesshconfig"
      "/home/${config.settings.user.admin.username}/.config/kwalletrc"
      "/home/${config.settings.user.admin.username}/.config/kwinoutputconfig.json"
      "/home/${config.settings.user.admin.username}/.config/kwinrc"
      "/home/${config.settings.user.admin.username}/.config/menu"
      "/home/${config.settings.user.admin.username}/.config/plasma-org.kde.plasma.desktop-appletsrc"
      "/home/${config.settings.user.admin.username}/.config/plasmashellrc"
      "/home/${config.settings.user.admin.username}/.config/QtProject.conf"
      "/home/${config.settings.user.admin.username}/.config/systemsettingsrc"
      "/home/${config.settings.user.admin.username}/.config/Trolltech.conf"
      "/home/${config.settings.user.admin.username}/.gtkrc-2.0"
      "/home/${config.settings.user.admin.username}/.local/share/baloo/index-lock"
      "/home/${config.settings.user.admin.username}/.local/share/kactivitymanagerd"
      "/home/${config.settings.user.admin.username}/.local/share/recently-used.xbel"
      "/home/${config.settings.user.admin.username}/.local/state/konsolestaterc"
      "/home/${config.settings.user.admin.username}/.local/state/systemsettingsstaterc"
      ##### Kwallet #####
      "/home/${config.settings.user.admin.username}/.local/share/kwalletd"
      ##### Klipper #####
      "/home/${config.settings.user.admin.username}/.local/share/klipper/history2.lst"
      ##### 1Password #####
      "/home/${config.settings.user.admin.username}/.config/1Password"
      ##### Chrome #####
      "/home/${config.settings.user.admin.username}/.config/google-chrome"
      "/home/${config.settings.user.admin.username}/.local/share/applications"
      "/home/${config.settings.user.admin.username}/.local/share/icons"
      "/home/${config.settings.user.admin.username}/.local/share/desktop-directories"
      ##### VSCode #####
      "/home/${config.settings.user.admin.username}/.config/Code"
      "/home/${config.settings.user.admin.username}/.vscode"
      ##### Sunshine #####
      "/home/${config.settings.user.admin.username}/.config/sunshine"
    ];
    ##### Boot ##### 
    settings.boot.method = mkSetting (enum [ "Disk-Operation" "Standard-Boot" "Secure-Boot"]) "Standard-Boot";
    settings.boot.pkiBundle = mkSetting str "/var/lib/sbctl";
    settings.boot.previousGenerationLimit = mkSetting int 3;
    settings.boot.timeout = mkSetting int 3;
    ##### TPM ##### 
    settings.tpm.device = mkSetting str "/dev/tpmrm0";
    settings.tpm.versionPath = mkSetting str "/sys/class/tpm/tpm0/tpm_version_major";
    ##### Desktop ##### 
    settings.desktop.environment = mkSetting (enum [ "hyprland-wayland" "plasma-wayland" "plasma-x11" "gnome-wayland" "gnome-x11"]) "plasma-wayland";
    settings.desktop.scalingFactor = mkSetting float 2.0;
    ##### Networking ##### 
    settings.networking.lanSubnet = mkSetting str "192.168.1.0/24"; # ip -o -f inet addr show | awk '/scope global/ {print $4}';
    settings.networking.ports.udp = mkSetting (listOf int) [];
    settings.networking.ports.tcp = mkSetting (listOf int) [];
    settings.networking.identityAgent = mkSetting str "~/.1password/agent.sock";
  };
}