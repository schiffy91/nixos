# Build a ready-to-boot UEFI raw disk image from an existing VM host configuration.
# The image boots via systemd-repart UKI and grows its root partition on first boot
# to use whatever sparse qcow2 size the hypervisor provisions.
{ config, lib, pkgs, modulesPath, ... }:
let
  arch = pkgs.stdenv.hostPlatform.parsed.cpu.name;
  efi = lib.toUpper pkgs.stdenv.hostPlatform.efiArch;
  partLabel = "${lib.toLower config.networking.hostName}-root";
  closure = pkgs.closureInfo { rootPaths = [ config.system.build.toplevel ]; };
in {
  imports = [ (modulesPath + "/image/repart.nix") ];

  boot.loader.grub.enable = lib.mkForce false;
  boot.loader.systemd-boot.enable = lib.mkForce false;
  boot.growPartition = true;

  # Seed an initial locked password hash on first boot of a prebuilt VM image
  # before passwordFile runs, without overwriting a password set later.
  system.activationScripts.vmSeedPasswordHash = {
    deps = [ "specialfs" ];
    text = ''
      if [ ! -e ${lib.escapeShellArg config.settings.users.admin.hashedPasswordFile} ]; then
        ${pkgs.coreutils}/bin/install -d -o root -g root -m 0700 "$(${pkgs.coreutils}/bin/dirname ${lib.escapeShellArg config.settings.users.admin.hashedPasswordFile})"
        printf '!\n' > ${lib.escapeShellArg config.settings.users.admin.hashedPasswordFile}
        ${pkgs.coreutils}/bin/chown root:root ${lib.escapeShellArg config.settings.users.admin.hashedPasswordFile}
        ${pkgs.coreutils}/bin/chmod 0600 ${lib.escapeShellArg config.settings.users.admin.hashedPasswordFile}
      fi
    '';
  };
  system.activationScripts.passwordFile.deps = [ "vmSeedPasswordHash" ];

  # Replace disko's multi-subvolume layout with a single auto-growing root partition
  # for prebuilt VM raw images while keeping the host's 9p /mnt/shared mount.
  fileSystems = lib.mkForce {
    "/" = {
      device = "/dev/disk/by-partlabel/${partLabel}";
      fsType = "ext4";
      autoResize = true;
      neededForBoot = true;
    };
    "/mnt/shared" = {
      device = "share";
      fsType = "9p";
      options = [
        "trans=virtio"
        "version=9p2000.L"
        "rw"
        "nofail"
        "x-systemd.automount"
      ];
    };
  };
  swapDevices = lib.mkForce [ ];

  systemd.services.register-nix-paths = {
    description = "Register the preinstalled NixOS closure";
    unitConfig = {
      DefaultDependencies = false;
      ConditionPathExists = "/nix-path-registration";
    };
    wantedBy = [ "sysinit.target" ];
    before = [ "sysinit.target" "shutdown.target" "nix-daemon.socket" "nix-daemon.service" ];
    after = [ "local-fs.target" ];
    conflicts = [ "shutdown.target" ];
    serviceConfig.Type = "oneshot";
    script = ''
      ${config.nix.package}/bin/nix-store --load-db < /nix-path-registration
      ${config.nix.package}/bin/nix-env -p /nix/var/nix/profiles/system --set /run/current-system
      touch /etc/NIXOS
      rm /nix-path-registration
    '';
  };

  image.repart = {
    enable = true;
    name = "${lib.toLower config.networking.hostName}-${arch}";
    sectorSize = 512;
    partitions = {
      "10-esp" = {
        contents."/EFI/BOOT/BOOT${efi}.EFI".source =
          "${config.system.build.uki}/${config.system.boot.loader.ukiFile}";
        repartConfig = {
          Type = "esp";
          Format = "vfat";
          SizeMinBytes = "512M";
        };
      };
      "20-root" = {
        contents."/nix-path-registration".source = "${closure}/registration";
        storePaths = [ config.system.build.toplevel ];
        repartConfig = {
          Type = "root";
          Format = "ext4";
          Label = partLabel;
          SizeMinBytes = "16G";
        };
      };
    };
  };
}
