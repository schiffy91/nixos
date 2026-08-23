{ lib, config, pkgs, ... }: 
let
  mkSetting = type: defaultValue: lib.mkOption { inherit type; default = defaultValue; };
  snapshotVolume = lib.lists.findFirst (volume: volume.flag == "snapshots") null config.settings.disk.subvolumes.volumes;
  adminHome = "/home/${config.settings.users.admin.username}";
  agentHome = "/home/${config.settings.users.agent.username}";
in {
  options = with lib.types; {
    ##### Secrets ##### 
    settings.secrets.path = mkSetting str "/etc/nixos/secrets";
    settings.secrets.hashedPasswordFile = mkSetting str "hashed_password.txt";
    ##### Admin ##### 
    settings.users.admin.username = mkSetting str "alexanderschiffhauer";
    settings.users.admin.publicName = mkSetting str "Alexander Schiffhauer";
    settings.users.admin.publicEmail = mkSetting str "Alexander.Schiffhauer@gmail.com";
    settings.users.admin.autoLogin.enable = mkSetting bool false;
    settings.users.admin.autoLock.enable = mkSetting bool true;
    settings.users.admin.autoUnlockWallet.enabled = mkSetting bool true;
    settings.users.admin.authorizedKeys = mkSetting (listOf str) [
      "ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAINJsoluI1m5T4iwuCbpSdHvLVdemN3v7wMrqk4e+XJA0"
    ];
    settings.users.admin.extraGroups = mkSetting (listOf str) [ "wheel" "networkmanager" ];
    settings.users.admin.homeManager.enable = mkSetting bool true;
    settings.users.mutable = mkSetting bool false;
    ##### Agent #####
    settings.users.agent.enable = mkSetting bool true;
    settings.users.agent.username = mkSetting str "agent";
    settings.users.agent.publicName = mkSetting str "Agent";
    settings.users.agent.publicEmail = mkSetting str "agent@localhost";
    settings.users.agent.authorizedKeys = mkSetting (listOf str) config.settings.users.admin.authorizedKeys;
    settings.users.agent.extraGroups = mkSetting (listOf str) [];
    settings.users.agent.homeManager.enable = mkSetting bool true;
    settings.users.agent.packages = mkSetting (listOf package) (with pkgs; [
      bashInteractive
      coreutils
      git
      ripgrep
      fd
      jq
      curl
      wget
      gnumake
      gcc
      python313
      nodejs
      nixd
      shellcheck
    ]);
    settings.users.agent.persist.paths = mkSetting (listOf str) [
      "${agentHome}/work"
      "${agentHome}/Downloads"
      "${agentHome}/.bash_history"
      "${agentHome}/.ssh/known_hosts"
      "${agentHome}/.cache"
      "${agentHome}/.config"
      "${agentHome}/.local/bin"
      "${agentHome}/.local/share"
      "${agentHome}/.local/state"
      "${agentHome}/.npm"
      "${agentHome}/.cargo"
      "${agentHome}/.codex"
      "${agentHome}/.claude"
      "${agentHome}/.claude.json"
      "${agentHome}/.gemini"
    ];
    ##### Disk ##### 
    settings.disk.device = mkSetting str "";
    settings.disk.boot.size = mkSetting str "4G";
    ##### Disk: Labels #####
    settings.disk.boot.efiSysMountPoint = mkSetting str "/boot";
    settings.disk.label.disk = mkSetting str "disk";
    settings.disk.label.main = mkSetting str "main";
    settings.disk.label.boot = mkSetting str "boot";
    settings.disk.label.recovery = mkSetting str "recovery";
    settings.disk.label.root = mkSetting str "root";
    settings.disk.partlabel.boot = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.boot}";
    settings.disk.partlabel.recovery = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.recovery}";
    settings.disk.partlabel.root = mkSetting str "${config.settings.disk.label.disk}-${config.settings.disk.label.main}-${config.settings.disk.label.root}";
    settings.disk.by.partlabel.boot = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.boot}";
    settings.disk.by.partlabel.recovery = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.recovery}";
    settings.disk.by.partlabel.root = mkSetting str "/dev/disk/by-partlabel/${config.settings.disk.partlabel.root}";
    settings.disk.by.mapper.root = mkSetting str "/dev/mapper/${config.settings.disk.label.root}";
    ##### Disk: Recovery #####
    settings.disk.recovery.enable = mkSetting bool false;
    settings.disk.recovery.size = mkSetting str "4G";
    settings.disk.recovery.mountPoint = mkSetting str "/recovery";
    settings.disk.recovery.filesystemLabel = mkSetting str "RECOVERY";
    settings.disk.recovery.kernelPath = mkSetting str "EFI/recovery/kernel.efi";
    settings.disk.recovery.initrdPath = mkSetting str "EFI/recovery/initrd";
    settings.disk.recovery.entryPath = mkSetting str "loader/entries/nixos-recovery.conf";
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
    settings.disk.subvolumes.snapshots.name = mkSetting str (toString snapshotVolume.name);
    settings.disk.subvolumes.snapshots.mountPoint = mkSetting str (toString snapshotVolume.mountPoint);
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
    settings.disk.immutability.mode = mkSetting (enum [ "converge" "reset" "prepare-only" "publish-prepared" "snapshot-only" "restore-a" "restore-b" "restore-c" "restore-generation" "restore-previous" "restore-penultimate" "disabled" ]) "converge";
    settings.disk.immutability.dryRun = mkSetting bool false;
    settings.disk.immutability.enforce.onReboot = mkSetting bool true;
    settings.disk.immutability.nonPersistedGenerations = mkSetting ints.unsigned 3;
    settings.disk.immutability.restoreGeneration = mkSetting ints.positive 1;
    settings.disk.persistence.enable = mkSetting bool true;
    settings.disk.immutability.persist.subvolumeRoot = mkSetting str "@persist";
    settings.disk.immutability.persist.snapshots.cleanName = mkSetting str "CLEAN";
    settings.disk.immutability.persist.paths = mkSetting (listOf str) ([
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
      "${adminHome}/Downloads"
      "${adminHome}/.bash_history"
      "${adminHome}/.config/dconf/user"
      "${adminHome}/.config/xsettingsd/xsettingsd.conf"
      "${adminHome}/.cache"
      "${adminHome}/.pki"
      ##### SSH #####
      "${adminHome}/.ssh/known_hosts"
      ##### Network Manager #####
      "${adminHome}/.cert/nm-openvpn"
      ##### Direnv #####
      "${adminHome}/.local/share/direnv"
      ##### Plasma #####
      "${adminHome}/.config/gtk-3.0"
      "${adminHome}/.config/gtk-4.0"
      "${adminHome}/.config/gtkrc-2.0"
      "${adminHome}/.config/gtkrc"
      "${adminHome}/.icons"
      "${adminHome}/.config/kcmfonts"
      "${adminHome}/.config/kcminputrc"
      "${adminHome}/.config/kdedefaults"
      "${adminHome}/.config/kdeglobals"
      "${adminHome}/.config/konsolesshconfig"
      "${adminHome}/.config/kwalletrc"
      "${adminHome}/.config/kwinoutputconfig.json"
      "${adminHome}/.config/kwinrc"
      "${adminHome}/.config/menus"
      "${adminHome}/.config/plasma-org.kde.plasma.desktop-appletsrc"
      "${adminHome}/.config/plasmashellrc"
      "${adminHome}/.config/QtProject.conf"
      "${adminHome}/.config/systemsettingsrc"
      "${adminHome}/.config/Trolltech.conf"
      "${adminHome}/.gtkrc-2.0"
      "${adminHome}/.local/share/kactivitymanagerd"
      "${adminHome}/.local/share/recently-used.xbel"
      "${adminHome}/.local/state/konsolestaterc"
      "${adminHome}/.local/state/systemsettingsstaterc"
      ##### Kwallet #####
      "${adminHome}/.local/share/kwalletd"
      ##### Klipper #####
      "${adminHome}/.local/share/klipper/history2.lst"
      ##### 1Password #####
      "${adminHome}/.config/1Password"
      ##### Chrome #####
      "${adminHome}/.config/google-chrome"
      "${adminHome}/.local/share/applications"
      "${adminHome}/.local/share/icons"
      "${adminHome}/.local/share/desktop-directories"
      ##### VSCode #####
      "${adminHome}/.config/Code"
      "${adminHome}/.vscode"
      ##### Codex #####
      "${adminHome}/.codex"
      ##### Agents #####
      "${adminHome}/.claude"
      "${adminHome}/.claude.json"
      "${adminHome}/.gemini"
      ##### Sunshine #####
      "${adminHome}/.config/sunshine"
      "${adminHome}/.local/share/flatpak/db"
      ##### Mullvad #####
      "/etc/mullvad-vpn/"
      "${adminHome}/.config/Mullvad VPN"
      ##### Steam #####
      "${adminHome}/.local/share/Steam"
      "${adminHome}/.steam"
      ##### Games #####
      "${adminHome}/Games"
      "${adminHome}/.local/share/containers"
      "${adminHome}/.local/share/umu"
      ##### Apple Music #####
      "${adminHome}/.config/sh.cider.genten"
      ##### rclone #####
      "${adminHome}/.config/rclone"
    ] ++ lib.optionals config.settings.users.agent.enable config.settings.users.agent.persist.paths);
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
    settings.desktop.enable = mkSetting bool true;
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
    ##### Apps #####
    settings.apps.enable = mkSetting bool true;
    settings.apps.dev.enable = mkSetting bool true;
    settings.apps.gaming.enable = mkSetting bool true;
    settings.apps.utils.enable = mkSetting bool true;
    settings.apps.agents.enable = mkSetting bool true;
    settings.apps.bash.enable = mkSetting bool true;
    settings.apps.battlenet.enable = mkSetting bool true;
    settings.apps.claude.enable = mkSetting bool true;
    settings.apps.codex.enable = mkSetting bool true;
    settings.apps.git.enable = mkSetting bool true;
    settings.apps.nixosctl.enable = mkSetting bool true;
    settings.apps.onePassword.enable = mkSetting bool true;

    settings.apps.rclone.enable = mkSetting bool true;
    settings.apps.rocksmith.enable = mkSetting bool true;
    settings.apps.steam.enable = mkSetting bool true;
    settings.apps.sunshine.enable = mkSetting bool true;
    settings.apps.sunshine.virtualDisplay.name = mkSetting str "sunshine-vmon";
    settings.apps.sunshine.virtualDisplay.disablePrimaryOnStream = mkSetting bool false;
    settings.apps.sunshine.virtualDisplay.primaryOutput = mkSetting str "";
    settings.apps.sunshine.virtualDisplay.position = mkSetting str "0,0";
    settings.apps.sunshine.virtualDisplay.scale = mkSetting str "1";
    settings.apps.sunshine.virtualDisplay.port = mkSetting port 5905;
    settings.apps.sunshine.virtualDisplay.defaultWidth = mkSetting ints.positive 1920;
    settings.apps.sunshine.virtualDisplay.defaultHeight = mkSetting ints.positive 1080;
    settings.apps.sunshine.virtualDisplay.defaultFps = mkSetting ints.positive 60;
    settings.apps.sunshine.virtualDisplay.settleSeconds = mkSetting ints.positive 3;
    settings.apps.vscode.enable = mkSetting bool true;
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
    settings.rocksmith.cdlcPath = mkSetting str "${adminHome}/Games/Rocksmith/cdlc";
    settings.rocksmith.slopsmith.configPath = mkSetting str "${adminHome}/Games/Rocksmith/slopsmith/config";
    settings.rocksmith.slopsmith.port = mkSetting port 8000;
    ##### nixosctl #####
    settings.nixosctl.configPath = mkSetting str "";
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
      nixos         = true;
      systemctl     = true;
      immutability  = true;
    };
    settings.sudolessAllowlist.packages = mkSetting (attrsOf bool) {
      tcpdump       = true;
      ethtool       = true;
      python3       = true;
      moonlight-qt  = true;
    };
  };
}
