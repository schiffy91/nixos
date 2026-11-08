{ inputs, config, immutabilityPersistKey, bootSafety, lib, pkgs, ... }:
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
  recoveryUkiPath = "EFI/recovery/nixos-recovery.efi";
  publicKey = "${config.settings.boot.pkiBundle}/keys/db/db.pem";
  privateKey = "${config.settings.boot.pkiBundle}/keys/db/db.key";
  signingRequired = config.settings.boot.method == "Secure-Boot";
  systemStateVersion = config.system.stateVersion;
  nixosMount = import ./recovery/mount.nix { inherit config immutabilityPersistKey inputs lib pkgs; };
  touchKeyboard = config.settings.boot.touchKeyboard;
  verifyModules = config.settings.boot.verifyModules;
  lockdown = config.settings.boot.lockdown;

  recoverySystem = lib.nixosSystem {
    system = pkgs.stdenv.hostPlatform.system;
    specialArgs = { inherit inputs; };
    modules = [
      "${inputs.nixpkgs}/nixos/modules/profiles/minimal.nix"
      "${inputs.nixpkgs}/nixos/modules/profiles/installation-device.nix"
      "${inputs.nixpkgs}/nixos/modules/installer/netboot/netboot.nix"
      ../../settings.nix
      ./boot/kernel-trust.nix
      ({ config, lib, pkgs, ... }: {
        networking.hostName = "nixos-recovery";
        services.buffyboard.enable = touchKeyboard;  # recovery is a console too
        settings.boot = { inherit verifyModules lockdown; };

        boot = {
          uki = {
            name = "nixos-recovery";
            version = null;
            settings.UKI.Initrd = lib.mkForce "${config.system.build.netbootRamdisk}/initrd";
          };
          initrd.availableKernelModules = virtioInitrdModules;
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

        netboot.squashfsCompression = "zstd -Xcompression-level 15";
        environment.systemPackages = [ nixosMount ];
      })
      # nixos-mount exposes the admin-owned flake at /mnt for root's nixos-install.
      (import ./images/environment.nix {
        authorizedKeys = adminAuthorizedKeys;
        stateVersion = systemStateVersion;
        safeGitDirectories = [ "/mnt/etc/nixos" ];
      })
    ];
  };

  recoveryToplevel = recoverySystem.config.system.build.toplevel;
  recoveryKernel = "${recoverySystem.config.system.build.kernel}/${recoverySystem.config.system.boot.loader.kernelFile}";
  recoveryInitrd = "${recoverySystem.config.system.build.netbootRamdisk}/initrd";
  recoverySystemdBoot = "${pkgs.systemd}/lib/systemd/boot/efi/systemd-boot${efiArch}.efi";
  recoveryUki = "${recoverySystem.config.system.build.uki}/${recoverySystem.config.system.boot.loader.ukiFile}";
  recoveryOptions = "init=${recoveryToplevel}/init ${toString recoverySystem.config.boot.kernelParams}";
  recoveryEntry = pkgs.writeText "nixos-recovery.conf" ''
    title NixOS Recovery
    sort-key z_recovery
    ${if signingRequired then ''
    efi /${recoveryUkiPath}
    '' else ''
    linux /${config.settings.disk.recovery.kernelPath}
    initrd /${config.settings.disk.recovery.initrdPath}
    options ${recoveryOptions}
    ''}
  '';
  recoveryLoaderConf = pkgs.writeText "loader.conf" ''
    default nixos-recovery.conf
    timeout 3
    editor no
  '';
  recoveryReadme = pkgs.writeText "nixos-recovery-README.txt" ''
    NixOS recovery partition

    Standard-Boot installs a split NixOS recovery kernel and initrd.
    Secure-Boot signs a unified recovery image, authenticating its kernel,
    initrd and command line, and signs the recovery fallback loader.
    The recovery partition also has its own fallback systemd-boot copy at the
    firmware fallback path for direct firmware boot.
  '';
  recoveryArtifactTriggers = [ recoveryToplevel recoveryKernel recoveryInitrd ] ++ lib.optional signingRequired recoveryUki;
  artifact = source: target: signed: { inherit source target signed; };
  boot = config.settings.disk.boot.efiSysMountPoint;
  recovery = config.settings.disk.recovery.mountPoint;
  payloads = if signingRequired then [
    (artifact recoveryUki "${boot}/${recoveryUkiPath}" true)
    (artifact recoveryUki "${recovery}/${recoveryUkiPath}" true)
  ] else lib.concatMap (root: [
    (artifact recoveryKernel "${root}/${config.settings.disk.recovery.kernelPath}" false)
    (artifact recoveryInitrd "${root}/${config.settings.disk.recovery.initrdPath}" false)
  ]) [ boot recovery ];
  artifactPlan = pkgs.writeText "nixos-recovery-artifacts.json" (builtins.toJSON {
    inherit boot recovery signingRequired publicKey privateKey;
    bootSafety = lib.optionalString (!signingRequired) (toString bootSafety);
    artifacts = payloads ++ [
      (artifact recoverySystemdBoot "${recovery}/${fallbackBootPath}" signingRequired)
      (artifact recoveryEntry "${boot}/${config.settings.disk.recovery.entryPath}" false)
      (artifact recoveryEntry "${recovery}/${config.settings.disk.recovery.entryPath}" false)
      (artifact recoveryLoaderConf "${recovery}/loader/loader.conf" false)
      (artifact recoveryReadme "${recovery}/README.txt" false)
    ];
    obsolete = lib.concatMap (root: if signingRequired then [
      "${root}/${config.settings.disk.recovery.kernelPath}"
      "${root}/${config.settings.disk.recovery.initrdPath}"
    ] else [ "${root}/${recoveryUkiPath}" ]) [ boot recovery ];
  });
  artifactInstaller = import ./recovery/artifacts.nix { inherit pkgs inputs; };
  installRecoveryScript = strict:
    "${artifactInstaller}/bin/nixos-recovery-artifacts ${artifactPlan} ${if strict then "--strict" else "--optional"}";

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

  system.build.recoverySystem = recoverySystem;  # so checks can assert on what recovery boots into
}
