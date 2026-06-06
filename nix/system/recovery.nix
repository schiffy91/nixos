{ inputs, config, lib, pkgs, ... }:
let
  efiArch = pkgs.stdenv.hostPlatform.efiArch;
  serialConsole =
    if pkgs.stdenv.hostPlatform.isAarch64
    then "ttyAMA0,115200n8"
    else "ttyS0,115200n8";
  virtioInitrdModules =
    if pkgs.stdenv.hostPlatform.isAarch64
    then [ "virtio_pci" "virtio_blk" "virtio_net" "virtio_mmio" "virtio_rng" ]
    else [ "virtio_pci" "virtio_blk" "virtio_net" "virtio_rng" ];
  fallbackBootPath = "EFI/BOOT/BOOT${lib.toUpper efiArch}.EFI";
  legacyUkiPath = "EFI/recovery/nixos-recovery.efi";
  publicKey = "${config.settings.boot.pkiBundle}/keys/db/db.pem";
  privateKey = "${config.settings.boot.pkiBundle}/keys/db/db.key";
  signingRequired = config.settings.boot.method == "Secure-Boot";

  recoverySystem = lib.nixosSystem {
    system = pkgs.stdenv.hostPlatform.system;
    modules = [
      "${inputs.nixpkgs}/nixos/modules/profiles/minimal.nix"
      "${inputs.nixpkgs}/nixos/modules/profiles/installation-device.nix"
      "${inputs.nixpkgs}/nixos/modules/installer/netboot/netboot.nix"
      ({ lib, pkgs, ... }: {
        networking.hostName = "nixos-recovery";
        system.stateVersion = config.system.stateVersion;

        boot = {
          initrd = {
            systemd.enable = true;
            availableKernelModules = virtioInitrdModules;
          };
          kernelParams = [
            "boot.shell_on_fail"
            "console=${serialConsole}"
            "console=tty0"
          ];
          supportedFilesystems = [ "btrfs" "vfat" "exfat" "ntfs" ];
        };

        documentation.enable = lib.mkDefault false;
        netboot.squashfsCompression = "zstd -Xcompression-level 15";

        nix.settings.experimental-features = [ "nix-command" "flakes" ];
        services.openssh = {
          enable = true;
          settings.PermitRootLogin = "prohibit-password";
        };
        users.users.root.openssh.authorizedKeys.keys =
          config.settings.users.admin.authorizedKeys;

        environment.systemPackages = with pkgs; [
          btrfs-progs
          cryptsetup
          curl
          efibootmgr
          git
          gptfdisk
          jq
          kexec-tools
          parted
          pciutils
          rsync
          sbctl
          smartmontools
          vim
        ];
      })
    ];
  };

  recoveryKernel = "${recoverySystem.config.system.build.kernel}/${recoverySystem.config.system.boot.loader.kernelFile}";
  recoveryInitrd = "${recoverySystem.config.system.build.netbootRamdisk}/initrd";
  recoverySystemdBoot = "${pkgs.systemd}/lib/systemd/boot/efi/systemd-boot${efiArch}.efi";
  recoveryOptions = "init=${recoverySystem.config.system.build.toplevel}/init ${toString recoverySystem.config.boot.kernelParams}";
  recoveryEntry = pkgs.writeText "nixos-recovery.conf" ''
    title NixOS Recovery
    sort-key z_recovery
    linux /${config.settings.disk.recovery.kernelPath}
    initrd /${config.settings.disk.recovery.initrdPath}
    options ${recoveryOptions}
  '';
  recoveryLoaderConf = pkgs.writeText "loader.conf" ''
    default nixos-recovery.conf
    timeout 3
    editor no
  '';
  recoveryReadme = pkgs.writeText "nixos-recovery-README.txt" ''
    NixOS recovery partition

    This partition contains the same split NixOS recovery kernel and initrd
    that the installed systemd-boot/lanzaboote menu loads from the ESP. The
    recovery partition also has its own fallback systemd-boot copy at the
    firmware fallback path for direct firmware boot.
  '';
in
lib.mkIf config.settings.disk.recovery.enable {
  systemd.services.nixos-recovery-boot-entry = {
    description = "Install NixOS recovery boot artifacts";
    wantedBy = [ "multi-user.target" ];
    after = [ "local-fs.target" "generate-sb-keys.service" ];
    restartTriggers = [
      recoveryKernel
      recoveryInitrd
      recoverySystemdBoot
      recoveryEntry
      recoveryLoaderConf
      recoveryReadme
    ];
    unitConfig = {
      RequiresMountsFor = [
        config.settings.disk.boot.efiSysMountPoint
        config.settings.disk.recovery.mountPoint
      ];
      AssertPathIsMountPoint = [
        config.settings.disk.boot.efiSysMountPoint
        config.settings.disk.recovery.mountPoint
      ];
    };
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = true;
    };
    path = with pkgs; [ coreutils sbsigntool ];
    script = ''
      set -euo pipefail
      umask 077

      recovery=${lib.escapeShellArg config.settings.disk.recovery.mountPoint}
      boot=${lib.escapeShellArg config.settings.disk.boot.efiSysMountPoint}
      public_key=${lib.escapeShellArg publicKey}
      private_key=${lib.escapeShellArg privateKey}
      signing_required=${lib.escapeShellArg (if signingRequired then "1" else "0")}

      if [ "$signing_required" = 1 ] && { [ ! -e "$public_key" ] || [ ! -e "$private_key" ]; }; then
        echo "Secure-Boot recovery artifacts require sbctl db keys at $public_key and $private_key" >&2
        exit 1
      fi

      install_signed() {
        source="$1"
        target="$2"
        mkdir -p "$(dirname "$target")"
        tmp="$(mktemp)"
        if [ -e "$public_key" ] && [ -e "$private_key" ]; then
          sbsign --key "$private_key" --cert "$public_key" --output "$tmp" "$source"
        else
          cp "$source" "$tmp"
        fi
        install -m 0644 "$tmp" "$target"
        rm -f "$tmp"
      }

      install_plain() {
        source="$1"
        target="$2"
        mkdir -p "$(dirname "$target")"
        install -m 0644 "$source" "$target"
      }

      install_signed ${lib.escapeShellArg recoveryKernel} "$boot/${config.settings.disk.recovery.kernelPath}"
      install_plain ${lib.escapeShellArg recoveryInitrd} "$boot/${config.settings.disk.recovery.initrdPath}"
      install_signed ${lib.escapeShellArg recoveryKernel} "$recovery/${config.settings.disk.recovery.kernelPath}"
      install_plain ${lib.escapeShellArg recoveryInitrd} "$recovery/${config.settings.disk.recovery.initrdPath}"
      install_signed ${lib.escapeShellArg recoverySystemdBoot} "$recovery/${fallbackBootPath}"

      rm -f "$boot/${legacyUkiPath}" "$recovery/${legacyUkiPath}"
      install -Dm0644 ${lib.escapeShellArg recoveryEntry} "$boot/${config.settings.disk.recovery.entryPath}"
      install -Dm0644 ${lib.escapeShellArg recoveryEntry} "$recovery/${config.settings.disk.recovery.entryPath}"
      install -Dm0644 ${lib.escapeShellArg recoveryLoaderConf} "$recovery/loader/loader.conf"
      install -Dm0644 ${lib.escapeShellArg recoveryReadme} "$recovery/README.txt"
    '';
  };
}
