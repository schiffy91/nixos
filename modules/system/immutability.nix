{ config, lib, pkgs, ... }: 
let 
  initrdKernelModules = [ "btrfs"];
  device = config.settings.disk.by.partlabel.root;
  rootSubvolumeName = config.settings.disk.subvolumes.root.name;
  snapshotsSubvolumeName = config.settings.disk.subvolumes.snapshots.name;
  cleanRootSnapshotRelativePath = config.settings.disk.immutability.persist.snapshots.cleanRoot;
  pathsToKeep = lib.strings.concatStringsSep " " (map lib.strings.escapeShellArg config.settings.disk.immutability.persist.paths);
  rootDevice = "dev-disk-by\\x2dpartlabel-${config.settings.disk.label.disk}\\x2d${config.settings.disk.label.main}\\x2d${config.settings.disk.label.root}.device"; # JFC…
in 
lib.mkIf config.settings.disk.immutability.enable {
  fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
  boot.readOnlyNixStore = true;
  boot.initrd = {
    supportedFilesystems = [ "btrfs" ];
    kernelModules = initrdKernelModules;
    availableKernelModules = initrdKernelModules;
    systemd.services.immutability = {
      description = "Apply immutability on-boot by resetting the filesystem to the original BTRFS snapshot and copying symlinks and intentionally preserved files";
      wantedBy = [ "initrd.target" ];
      requires = [ rootDevice ];
      after = [ rootDevice "systemd-cryptsetup@${config.settings.disk.partlabel.root}.service" ];
      before = [ "sysroot.mount" ];
      unitConfig.DefaultDependencies = "no";
      serviceConfig.Type = "oneshot";
      script = ''
      ##################################################
      ##### Setup args and arg-dependent variables #####
      ##################################################
      set -euo pipefail
      # Create mount point if it doesn't exist
      MOUNT="/mnt"
      mkdir -p "$MOUNT"
      ##### Parse args #####
      DEVICE="${device}"                                          # /dev/disk/by-label/disk-main-root
      ROOT="$MOUNT/${rootSubvolumeName}"                          # /mnt/@root <--------------------  @root
      SNAPSHOTS="$MOUNT/${snapshotsSubvolumeName}"                # /mnt/@snapshots <---------------  @snapshots
      CLEAN_ROOT="$SNAPSHOTS/${cleanRootSnapshotRelativePath}"    # /mnt/@snapshots/CLEAN_ROOT <---- CLEAN_ROOT
      PATHS_TO_KEEP="${pathsToKeep}"                              # "/etc/nixos /etc/machine-id /home/alexanderschiffhauer"
      echo "MOUNT: $MOUNT"
      echo "ROOT: $ROOT"
      echo "SNAPSHOTS: $SNAPSHOTS"
      echo "CLEAN_ROOT: $CLEAN_ROOT"
      echo "PATHS_TO_KEEP: $PATHS_TO_KEEP"
      # Validate device exists
      if [ ! -b "$DEVICE" ]; then
          echo "Error: Device '$DEVICE' does not exist"
          exit 1
      fi                       
      ##### Mount subvolumes #####
      mount -t btrfs -o subvolid=5,user_subvol_rm_allowed "$DEVICE" "$MOUNT"
      ##### Validate CLEAN_ROOT exists #####
      if [ -z "$CLEAN_ROOT" ] || [ ! -d "$CLEAN_ROOT" ]; then
        echo "Error: '$CLEAN_ROOT' is not a directory or is empty"
        exit 1
      fi

      #################################
      ##### BTRFS Delete Function #####
      #################################
      btrfs_subvolume_delete_recursively() {
        local path="$1"
        echo "btrfs_subvolume_delete_recursively: $path"
        [ ! -d "$path" ] && echo "  Warning: $path does not exist" && return 0
        
        local subvolumes
        subvolumes=$(btrfs subvolume list -o "$path" | tac | cut -f 9- -d ' ')
        
        # Delete each nested subvolume
        IFS=$'\n'
        for subvolume in $subvolumes; do
            echo "  Deleting nested: $subvolume"
            btrfs subvolume delete "$MOUNT/$subvolume" || {
                echo "  Warning: Failed to delete $subvolume"
                continue
            }
            btrfs filesystem sync "$ROOT"
        done
        
        # Finally delete the main subvolume
        echo "  Deleting main: $path"
        btrfs subvolume delete "$path" && {
            btrfs filesystem sync "$ROOT"
            echo "  Deleted: $path"
        } || echo "  ERROR: Failed to delete $path"
      }

      #################################
      ##### BTRFS Copy Function #####
      #################################
      btrfs_subvolume_copy() {
        local source="$1"
        local target="$2"
        echo "btrfs_subvolume_copy: $source -> $target"
        
        [ ! -d "$source" ] && { 
            echo " ERROR: Source $source does not exist"
            return 1
        }
        
        btrfs_subvolume_delete_recursively "$target"
        btrfs subvolume snapshot "$source" "$target" || {
            echo " ERROR: Failed to create snapshot from $source to $target"
            return 1
        }
        btrfs filesystem sync "$ROOT"
      }

      ############################
      ##### Manage snapshots #####
      ############################
      ##### Create Snapshot path variables #####
      PREVIOUS_SNAPSHOT="$SNAPSHOTS/PREVIOUS_SNAPSHOT"              # /mnt/@snapshots/PREVIOUS_SNAPSHOT
      PENULTIMATE_SNAPSHOT="$SNAPSHOTS/PENULTIMATE_SNAPSHOT"        # /mnt/@snapshots/PENULTIMATE_SNAPSHOT
      CURRENT_SNAPSHOT="$SNAPSHOTS/CURRENT_SNAPSHOT"                # /mnt/@snapshots/CURRENT_SNAPSHOT
      ##### Setup PREVIOUS_SNAPSHOT and PENULTIMTE_SNAPSHOT if they don't exist #####
      btrfs_subvolume_copy "$CLEAN_ROOT" "$PENULTIMATE_SNAPSHOT"
      btrfs_subvolume_copy "$CLEAN_ROOT" "$PREVIOUS_SNAPSHOT"
      ##### Delete existing snapshots and create new ones #####
      btrfs_subvolume_copy "$PREVIOUS_SNAPSHOT" "$PENULTIMATE_SNAPSHOT"
      btrfs_subvolume_copy "$ROOT" "$PREVIOUS_SNAPSHOT"
      btrfs_subvolume_copy "$CLEAN_ROOT" "$CURRENT_SNAPSHOT"
      ##### Make the current snapshot read-writeable #####
      btrfs property set -ts "$CURRENT_SNAPSHOT" ro false 2>/dev/null || true

      ######################
      ##### Copy paths #####
      ######################
      ##### Copy the explicitly preserved paths into the current snapshot. #####
      echo "Copying '$PATHS_TO_KEEP' to '$CURRENT_SNAPSHOT'..."
      for path in $PATHS_TO_KEEP; do
        current_path="$ROOT/$path"
        tmp_path="$CURRENT_SNAPSHOT/$path"
        if [ -e "$current_path" ]; then
          mkdir -p "$(dirname "$tmp_path")"
          cp -a "$current_path" "$tmp_path"
          echo "Successfully copied '$current_path' to '$tmp_path'"
        else
          echo "Warning: '$current_path' does not exist and was not preserved."
        fi
      done
      ##### NixOS depends on an unfathomable amount of symlinks, so just copy all of them into the current snapshot. #####
      echo "Preserving new symlinks..."
      (
        cd "$ROOT"
        find . -type l | while read -r link; do
          if [ ! -e "$CURRENT_SNAPSHOT/$link" ]; then
            mkdir -p "$(dirname "$CURRENT_SNAPSHOT/$link")"
            cp -a "$link" "$CURRENT_SNAPSHOT/$link"
            echo "Successfully copied '$link' to '$CURRENT_SNAPSHOT/$link'"
          fi
        done
      )

      #####################
      ##### Swap root #####
      #####################
      ##### If the power is plugged now, you can restore /mnt/@snapshots/PREVIOUS_SNAPSHOT #####
      ##### Re-create the root subvolume by creating a snapshot based on what we just constructed. #####
      echo "Re-creating '$ROOT' from '$CURRENT_SNAPSHOT'..."
      btrfs_subvolume_copy "$CURRENT_SNAPSHOT" "$ROOT"
      ##### Unmount & Delete Mountpoint #####
      umount "$MOUNT"
      rm -rf "$MOUNT"
      ##### Finish #####
      echo "Done. '$ROOT' has been restored to the state of '$CLEAN_ROOT' + new symlinks + keep-paths."
      '';
    };
  };
}