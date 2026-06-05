{ inputs, config, lib, pkgs, ... }:
let
  efiArch = pkgs.stdenv.hostPlatform.efiArch;
  fallbackBootPath = "EFI/BOOT/BOOT${lib.toUpper efiArch}.EFI";
  publicKey = "${config.settings.boot.pkiBundle}/keys/db/db.pem";
  privateKey = "${config.settings.boot.pkiBundle}/keys/db/db.key";

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
          initrd.systemd.enable = true;
          kernelParams = [
            "boot.shell_on_fail"
            "console=tty0"
          ];
          supportedFilesystems = [ "btrfs" "vfat" "exfat" "ntfs" ];
          uki = {
            name = "nixos-recovery";
            version = null;
            settings.UKI.Initrd = lib.mkForce "${recoverySystem.config.system.build.netbootRamdisk}/initrd";
          };
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

  recoveryUki = "${recoverySystem.config.system.build.uki}/${recoverySystem.config.system.boot.loader.ukiFile}";
  recoveryEntry = pkgs.writeText "nixos-recovery.conf" ''
    title NixOS Recovery
    efi /${config.settings.disk.recovery.efiPath}
    sort-key z_recovery
  '';
  recoveryLoaderConf = pkgs.writeText "nixos-recovery-loader.conf" ''
    default nixos-recovery.conf
    timeout 5
    editor no
  '';
  recoveryReadme = pkgs.writeText "nixos-recovery-README.txt" ''
    NixOS recovery partition

    This partition contains a self-contained NixOS recovery UKI generated from
    the repository's recovery module. The installed systemd-boot/lanzaboote menu
    loads EFI/recovery/nixos-recovery.efi. The fallback EFI boot path on this
    partition loads the same recovery entry directly from firmware.
  '';
in
lib.mkIf config.settings.disk.recovery.enable {
  systemd.services.nixos-recovery-boot-entry = {
    description = "Install NixOS recovery boot artifacts";
    wantedBy = [ "multi-user.target" ];
    after = [ "local-fs.target" "generate-sb-keys.service" ];
    restartTriggers = [
      recoveryUki
      recoveryEntry
      recoveryLoaderConf
      recoveryReadme
    ];
    unitConfig = {
      RequiresMountsFor = [
        config.settings.disk.boot.efiSysMountPoint
        config.settings.disk.recovery.mountPoint
      ];
      ConditionPathIsMountPoint = config.settings.disk.recovery.mountPoint;
    };
    serviceConfig = {
      Type = "oneshot";
      RemainAfterExit = true;
    };
    path = with pkgs; [ coreutils sbsigntool ];
    script = ''
      set -euo pipefail
      umask 077

      boot=${lib.escapeShellArg config.settings.disk.boot.efiSysMountPoint}
      recovery=${lib.escapeShellArg config.settings.disk.recovery.mountPoint}
      public_key=${lib.escapeShellArg publicKey}
      private_key=${lib.escapeShellArg privateKey}

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

      install_signed ${lib.escapeShellArg recoveryUki} "$boot/${config.settings.disk.recovery.efiPath}"
      install_signed ${lib.escapeShellArg recoveryUki} "$recovery/${config.settings.disk.recovery.efiPath}"
      install_signed ${lib.escapeShellArg "${config.systemd.package}/lib/systemd/boot/efi/systemd-boot${efiArch}.efi"} "$recovery/${fallbackBootPath}"

      install -Dm0644 ${lib.escapeShellArg recoveryEntry} "$boot/${config.settings.disk.recovery.entryPath}"
      install -Dm0644 ${lib.escapeShellArg recoveryEntry} "$recovery/${config.settings.disk.recovery.entryPath}"
      install -Dm0644 ${lib.escapeShellArg recoveryLoaderConf} "$recovery/loader/loader.conf"
      install -Dm0644 ${lib.escapeShellArg recoveryReadme} "$recovery/README.txt"
    '';
  };
}
