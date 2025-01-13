{ inputs, config, lib, pkgs, ... }:
let 
# This script is run at boot to reset /root from root-blank,
  # similar to your original Bash snippet:
  ephemeralRootScript = pkgs.writeScript "ephemeral-root.sh" ''
    #!${pkgs.bash}/bin/bash
    set -euo pipefail

    # Require root
    if [ "$UID" -ne "0" ]; then
      echo "Must run as root to manipulate btrfs subvolumes." >&2
      exit 1
    fi

    # Get access to the BTRFS volume
    BTRFS_VOL="/dev/disk/by-partlabel/disk-${config.settings.disk.label.main}-${config.settings.disk.label.root}"
    if [ ! -b "$BTRFS_VOL" ]; then
      echo "Device '$BTRFS_VOL' not found or not a block device" >&2
      exit 1
    fi

    # Mount BTRFS subvolume "/root" to a temporary directory
    MOUNTDIR=$(mktemp -d)
    mount -t btrfs -o subvol=${config.settings.disk.subvolumes."/root".mountPoint} "$BTRFS_VOL" "$MOUNTDIR"

    # If BTRF's “root” has sub-subvolumes, remove them  – otherwise btrfs complains
    ROOT_SUBVOL="$MOUNTDIR/root"
    btrfs subvolume list -o "$ROOT_SUBVOL" \
      | cut -f9 -d' ' \
      | while read -r sub; do
          echo "Deleting nested /$sub subvolume..."
          btrfs subvolume delete "$MOUNTDIR/$sub"
        done

    # Delete BTRF's /root subvolume   
    echo "Deleting /root subvolume..."
    btrfs subvolume delete "$ROOT_SUBVOL"

    # Restore an empty BTRF snapshot
    echo "Restoring blank /root subvolume..."
    BLANK_ROOT_SNAPSHOT="$MOUNTDIR/root-blank"
    btrfs subvolume snapshot "$BLANK_ROOT_SNAPSHOT" "$ROOT_SUBVOL"

    umount "$MOUNTDIR"
    rmdir "$MOUNTDIR"
  '';
in {
  imports = [ inputs.impermanence.nixosModules.impermanence ];
  fileSystems."${config.settings.disk.immutability.persist.mountPoint}".neededForBoot = true;
  boot.readOnlyNixStore = config.settings.disk.immutability.enable;
  environment.persistence."${config.settings.disk.immutability.persist.mountPoint}" = {
    enable = config.settings.disk.immutability.enable;
    directories = config.settings.disk.immutability.persist.directories;
    files = config.settings.disk.immutability.persist.files;
    hideMounts = true;
  };
  systemd.services.ephemeral-root = lib.mkIf config.settings.disk.immutability.enable {
    description = "Immutability based on BTRFS snapshots";
    wantedBy = if config.settings.disk.encryption.enable then [ "cryptsetup.target" ] else [ "local-fs.target" ];
    after = if config.settings.disk.encryption.enable then [ "cryptsetup.target" ] else [ "local-fs.target" ];
    serviceConfig = {
      Type = "oneshot";
      ExecStart = "${ephemeralRootScript}";
    };
  };
}