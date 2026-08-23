{ inputs, config, immutabilityPersistKey, lib, pkgs, ... }:
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
  adminAuthorizedKeys = config.settings.users.admin.authorizedKeys;
  fallbackBootPath = "EFI/BOOT/BOOT${lib.toUpper efiArch}.EFI";
  legacySecureRecoveryUkiPath = "EFI/recovery/nixos-recovery.efi";
  publicKey = "${config.settings.boot.pkiBundle}/keys/db/db.pem";
  privateKey = "${config.settings.boot.pkiBundle}/keys/db/db.key";
  signingRequired = config.settings.boot.method == "Secure-Boot";
  systemStateVersion = config.system.stateVersion;
  nixosMount = import ../recovery/mount.nix { inherit config immutabilityPersistKey lib pkgs; };

  recoverySystem = lib.nixosSystem {
    system = pkgs.stdenv.hostPlatform.system;
    modules = [
      "${inputs.nixpkgs}/nixos/modules/profiles/minimal.nix"
      "${inputs.nixpkgs}/nixos/modules/profiles/installation-device.nix"
      "${inputs.nixpkgs}/nixos/modules/installer/netboot/netboot.nix"
      ({ config, lib, pkgs, ... }: {
        networking.hostName = "nixos-recovery";
        system.stateVersion = systemStateVersion;

        boot = {
          initrd = {
            systemd.enable = true;
            availableKernelModules = virtioInitrdModules;
          };
          kernelParams = [
            "boot.shell_on_fail"
            "console=${serialConsole}"
            "console=tty0"
            "nomodeset"
            "module_blacklist=amdgpu,nouveau"
            "usbcore.initial_descriptor_timeout=1000"
            "systemd.show_status=1"
            "rd.systemd.show_status=1"
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
        services.getty.autologinUser = lib.mkForce "root";
        users.users.root.openssh.authorizedKeys.keys =
          adminAuthorizedKeys;

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
          nixosMount
        ];
      })
    ];
  };

  recoveryToplevel = recoverySystem.config.system.build.toplevel;
  recoveryKernel = "${recoverySystem.config.system.build.kernel}/${recoverySystem.config.system.boot.loader.kernelFile}";
  recoveryInitrd = "${recoverySystem.config.system.build.netbootRamdisk}/initrd";
  recoverySystemdBoot = "${pkgs.systemd}/lib/systemd/boot/efi/systemd-boot${efiArch}.efi";
  recoveryOptions = "init=${recoveryToplevel}/init ${toString recoverySystem.config.boot.kernelParams}";
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

    Standard-Boot installs a split NixOS recovery kernel and initrd.
    Secure-Boot signs the recovery kernel and recovery fallback loader.
    The recovery partition also has its own fallback systemd-boot copy at the
    firmware fallback path for direct firmware boot.
  '';
  recoveryArtifactTriggers = [ recoveryToplevel recoveryKernel recoveryInitrd ];
  installRecoveryArtifacts =
    if signingRequired
    then ''
      install_secure ${lib.escapeShellArg recoveryKernel} "$boot/${config.settings.disk.recovery.kernelPath}"
      install_secure ${lib.escapeShellArg recoveryKernel} "$recovery/${config.settings.disk.recovery.kernelPath}"
      install_plain ${lib.escapeShellArg recoveryInitrd} "$boot/${config.settings.disk.recovery.initrdPath}"
      install_plain ${lib.escapeShellArg recoveryInitrd} "$recovery/${config.settings.disk.recovery.initrdPath}"
    ''
    else ''
      install_plain ${lib.escapeShellArg recoveryKernel} "$boot/${config.settings.disk.recovery.kernelPath}"
      install_plain ${lib.escapeShellArg recoveryInitrd} "$boot/${config.settings.disk.recovery.initrdPath}"
      install_plain ${lib.escapeShellArg recoveryKernel} "$recovery/${config.settings.disk.recovery.kernelPath}"
      install_plain ${lib.escapeShellArg recoveryInitrd} "$recovery/${config.settings.disk.recovery.initrdPath}"
    '';
  removeLegacyRecoveryArtifacts = ''
    rm -f "$boot/${legacySecureRecoveryUkiPath}" "$recovery/${legacySecureRecoveryUkiPath}"
  '';
  installRecoveryFallback =
    if signingRequired
    then ''
      install_secure ${lib.escapeShellArg recoverySystemdBoot} "$recovery/${fallbackBootPath}"
    ''
    else ''
      install_plain ${lib.escapeShellArg recoverySystemdBoot} "$recovery/${fallbackBootPath}"
    '';
  installRecoveryScript = strict: ''
    install_nixos_recovery_boot_artifacts() {
      set -euo pipefail
      umask 077

      recovery=${lib.escapeShellArg config.settings.disk.recovery.mountPoint}
      boot=${lib.escapeShellArg config.settings.disk.boot.efiSysMountPoint}
      public_key=${lib.escapeShellArg publicKey}
      private_key=${lib.escapeShellArg privateKey}
      signing_required=${lib.escapeShellArg (if signingRequired then "1" else "0")}
      strict=${lib.escapeShellArg (if strict then "1" else "0")}

      if ! ${pkgs.util-linux}/bin/mountpoint -q "$boot" || ! ${pkgs.util-linux}/bin/mountpoint -q "$recovery"; then
        echo "Skipping recovery boot artifacts because $boot or $recovery is not mounted." >&2
        return 0
      fi

      if [ "$signing_required" = 1 ] && { [ ! -e "$public_key" ] || [ ! -e "$private_key" ]; }; then
        if [ "$strict" = 1 ]; then
          echo "Secure-Boot recovery artifacts require sbctl db keys at $public_key and $private_key" >&2
          return 1
        fi
        echo "Skipping Secure-Boot recovery artifacts because sbctl db keys are not available yet." >&2
        return 0
      fi

      ${pkgs.coreutils}/bin/install -d -m 1777 /tmp
      scratch=$(${pkgs.coreutils}/bin/mktemp -d /tmp/nixos-recovery-sign.XXXXXX)
      manifest="$scratch/manifest"
      installed_manifest="$recovery/.nixos-recovery-artifacts.manifest"

      write_manifest() {
        {
          printf 'signing_required=%s\n' "$signing_required"
          printf 'kernel=%s\n' ${lib.escapeShellArg recoveryKernel}
          printf 'initrd=%s\n' ${lib.escapeShellArg recoveryInitrd}
          printf 'systemd_boot=%s\n' ${lib.escapeShellArg recoverySystemdBoot}
          printf 'entry=%s\n' ${lib.escapeShellArg recoveryEntry}
          printf 'loader_conf=%s\n' ${lib.escapeShellArg recoveryLoaderConf}
          printf 'readme=%s\n' ${lib.escapeShellArg recoveryReadme}
          printf 'kernel_path=%s\n' ${lib.escapeShellArg config.settings.disk.recovery.kernelPath}
          printf 'initrd_path=%s\n' ${lib.escapeShellArg config.settings.disk.recovery.initrdPath}
          printf 'entry_path=%s\n' ${lib.escapeShellArg config.settings.disk.recovery.entryPath}
          printf 'fallback_path=%s\n' ${lib.escapeShellArg fallbackBootPath}
          if [ "$signing_required" = 1 ]; then
            ${pkgs.coreutils}/bin/sha256sum "$public_key"
          fi
        } > "$manifest"
      }

      artifacts_exist() {
        [ -f "$boot/${config.settings.disk.recovery.kernelPath}" ] &&
        [ -f "$boot/${config.settings.disk.recovery.initrdPath}" ] &&
        [ -f "$recovery/${config.settings.disk.recovery.kernelPath}" ] &&
        [ -f "$recovery/${config.settings.disk.recovery.initrdPath}" ] &&
        [ -f "$recovery/${fallbackBootPath}" ] &&
        [ -f "$boot/${config.settings.disk.recovery.entryPath}" ] &&
        [ -f "$recovery/${config.settings.disk.recovery.entryPath}" ] &&
        [ -f "$recovery/loader/loader.conf" ] &&
        [ -f "$recovery/README.txt" ]
      }

      write_manifest
      if artifacts_exist && [ -f "$installed_manifest" ] && ${pkgs.diffutils}/bin/cmp -s "$manifest" "$installed_manifest"; then
        echo "Recovery boot artifacts already current."
        ${pkgs.coreutils}/bin/rm -rf "$scratch"
        return 0
      fi

      install_secure() {
        source="$1"
        target="$2"
        ${pkgs.coreutils}/bin/mkdir -p "$(${pkgs.coreutils}/bin/dirname "$target")"
        tmp="$scratch/signed"
        ${pkgs.sbsigntool}/bin/sbsign --key "$private_key" --cert "$public_key" --output "$tmp" "$source"
        ${pkgs.coreutils}/bin/install -m 0644 "$tmp" "$target"
        ${pkgs.coreutils}/bin/rm -f "$tmp"
      }

      install_plain() {
        source="$1"
        target="$2"
        ${pkgs.coreutils}/bin/mkdir -p "$(${pkgs.coreutils}/bin/dirname "$target")"
        ${pkgs.coreutils}/bin/install -m 0644 "$source" "$target"
      }

      ${installRecoveryArtifacts}
      ${installRecoveryFallback}
      ${removeLegacyRecoveryArtifacts}

      ${pkgs.coreutils}/bin/install -Dm0644 ${lib.escapeShellArg recoveryEntry} "$boot/${config.settings.disk.recovery.entryPath}"
      ${pkgs.coreutils}/bin/install -Dm0644 ${lib.escapeShellArg recoveryEntry} "$recovery/${config.settings.disk.recovery.entryPath}"
      ${pkgs.coreutils}/bin/install -Dm0644 ${lib.escapeShellArg recoveryLoaderConf} "$recovery/loader/loader.conf"
      ${pkgs.coreutils}/bin/install -Dm0644 ${lib.escapeShellArg recoveryReadme} "$recovery/README.txt"
      ${pkgs.coreutils}/bin/install -Dm0644 "$manifest" "$installed_manifest"
      ${pkgs.coreutils}/bin/rm -rf "$scratch"
    }

    install_nixos_recovery_boot_artifacts
  '';
in
lib.mkIf config.settings.disk.recovery.enable {
  system.activationScripts.nixosRecoveryBootArtifacts = lib.stringAfter [ "etc" ] (installRecoveryScript false);

  systemd.services.nixos-recovery-boot-entry = {
    description = "Install NixOS recovery boot artifacts";
    wantedBy = [ "multi-user.target" ];
    after = [ "local-fs.target" "generate-sb-keys.service" ];
    restartTriggers = recoveryArtifactTriggers ++ [
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
    script = installRecoveryScript true;
  };
}
