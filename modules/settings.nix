{ lib, config, pkgs, ... }: 
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
    settings.user.admin.authorizedKeys = mkSetting (listOf str) [
      "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAINJsoluI1m5T4iwuCbpSdHvLVdemN3v7wMrqk4e+XJA0"
    ];
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
    settings.disk.immutability.mode = mkSetting (enum [ "reset" "snapshot-only" "restore-previous" "restore-penultimate" "disabled" ]) "reset";
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
      "/root/.ssh/known_hosts"
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
      "/home/${config.settings.user.admin.username}/.icons"
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
      "/home/${config.settings.user.admin.username}/.local/share/flatpak/db"
      ##### Mullvad #####
      "/etc/mullvad-vpn/"
      "/home/${config.settings.user.admin.username}/.config/Mullvad VPN"
      ##### Claude #####
      "/home/${config.settings.user.admin.username}/.claude"
      "/home/${config.settings.user.admin.username}/.claude.json"
      ##### Steam #####
      "/home/${config.settings.user.admin.username}/.local/share/Steam"
      "/home/${config.settings.user.admin.username}/.steam"
      ##### Games #####
      "/home/${config.settings.user.admin.username}/Games"
      "/home/${config.settings.user.admin.username}/.local/share/umu"
      ##### Apple Music #####
      "/home/${config.settings.user.admin.username}/.config/sh.cider.genten"
      ##### rclone #####
      "/home/${config.settings.user.admin.username}/.config/rclone"
    ];
    ##### Sleep #####
    settings.sleep.allowHibernation = mkSetting bool false;
    ##### Boot #####
    settings.boot.method = mkSetting (enum [ "Disk-Operation" "Standard-Boot" "Secure-Boot"]) "Standard-Boot";
    settings.boot.pkiBundle = mkSetting str "/var/lib/sbctl";
    settings.boot.previousGenerationLimit = mkSetting int 3;
    settings.boot.timeout = mkSetting int 3;
    ##### TPM ##### 
    settings.tpm.device = mkSetting str "/dev/tpmrm0";
    settings.tpm.versionPath = mkSetting str "/sys/class/tpm/tpm0/tpm_version_major";
    ##### Desktop #####
    settings.desktop.outputs = mkSetting (listOf (submodule {
      options = {
        name = mkSetting str null;
        scaleFactor = mkSetting float 1.0;
        primary = mkSetting bool false;
      };
    })) [];
    settings.desktop.cursor.theme = mkSetting str "Breeze";
    settings.desktop.cursor.package = mkSetting package pkgs.kdePackages.breeze;
    settings.desktop.cursor.path = mkSetting str "${config.settings.desktop.cursor.package}/share/icons/breeze_cursors";
    settings.desktop.cursor.defaultPackage = mkSetting package (pkgs.runCommandLocal "breeze-cursor-default-theme" { } ''
      mkdir -p "$out/share/icons"
      ln -s "${config.settings.desktop.cursor.path}" "$out/share/icons/default"
    '');
    ##### Desktop: Plasma #####
    settings.desktop.plasma.colorScheme = mkSetting str "BreezeDark";
    settings.desktop.plasma.iconTheme = mkSetting str "Papirus-Dark";
    settings.desktop.plasma.iconThemePackage = mkSetting package pkgs.papirus-icon-theme;
    settings.desktop.plasma.accentColor = mkSetting str "40,40,40";
    settings.desktop.plasma.wallpaper = mkSetting str "${pkgs.kdePackages.plasma-workspace-wallpapers}/share/wallpapers/Next/contents/images_dark/5120x2880.png";
    ##### Networking #####
    settings.networking.lanSubnet = mkSetting str "192.168.1.0/24"; # ip -o -f inet addr show | awk '/scope global/ {print $4}';
    settings.networking.ports.udp = mkSetting (listOf int) [];
    settings.networking.ports.tcp = mkSetting (listOf int) [];
    settings.networking.identityAgent = mkSetting str "~/.1password/agent.sock";
    settings.networking.primaryInterface = mkSetting str "";  # empty = no preference; set to e.g. "eno2" to route outbound via that NIC (prevents asymmetric routing when multiple NICs on one subnet)
    ##### Input #####
    settings.input.libinputMice = mkSetting (listOf (submodule {
      options = {
        vendorId = mkSetting str null;
        productId = mkSetting str null;
        name = mkSetting str null;
        accelProfile = mkSetting (enum [ "flat" "adaptive" ]) "flat";
      };
    })) [];
    ##### Rocksmith #####
    settings.rocksmith.sampleSize = mkSetting int 64;
    settings.rocksmith.sampleRate = mkSetting int 48000;
    ##### NixOS Helper #####
    settings.nixosHelper.configPath = mkSetting str "";
    ##### Sudoless Allowlist #####
    settings.sudolessAllowlist.enable   = mkSetting bool false;
    settings.sudolessAllowlist.nopasswd = mkSetting (attrsOf bool) {
      nixos-rebuild = true;
      tcpdump       = true;
      ethtool       = true;
      mount         = true;
      umount        = true;
      losetup       = true;
      "mkfs.btrfs"  = true;
      btrfs         = true;
      python3       = true;
    };
    settings.sudolessAllowlist.packages = mkSetting (attrsOf bool) {
      tcpdump     = true;
      ethtool     = true;
      python3     = true;
      moonlight-qt = true;
    };
  };
}