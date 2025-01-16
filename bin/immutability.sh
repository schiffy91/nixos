#!/usr/bin/env bash
# restore-subvolume.sh - revert a Btrfs subvolume from a snapshot, preserving user-specified paths and ignoring new symlinks.
set -euo pipefail

##################################################
##### Setup args and arg-dependent variables #####
##################################################
MOUNT="/mnt"
##### Check number of args #####
if [ $# -ne 5 ]; then
    echo "Error: Incorrect number of arguments"
    echo "Usage: $0 <device> <root_subvolume_name> <snapshots_subvolume_name> <clean_root_snapshot_relative_path> \"<paths_to_keep>\""
    echo "Example: $0 /dev/disk/by-partlabel/disk-main-root @root / @snapshots CLEAN_ROOT '/etc/nixos /home/alexanderschiffhauer'"
    exit 1
fi
##### Parse args #####
DEVICE="${1:?'Device parameter is required'}"                     # /dev/disk/by-label/disk-main-root
ROOT="${MOUNT}/${2:?'Root subvolume name is required'}"           # /mnt/@root <--------------------  @root
SNAPSHOTS="${3:?'Snapshots subvolume name is required'}"          # /mnt/@snapshots <---------------  @snapshots
CLEAN_ROOT="${SNAPSHOTS}/${4:?'Clean root path is required'}"     # /mnt/@snapshots/CLEAN_ROOT <---- CLEAN_ROOT
PATHS_TO_KEEP="${5:?'Paths to keep is required'}"                 # "/etc/nixos /etc/machine-id /home/alexanderschiffhauer"
# Validate device exists
if [ ! -b "$DEVICE" ]; then
    echo "Error: Device '$DEVICE' does not exist"
    exit 1
fi
# Ensure mount point is available
if mountpoint -q "${MOUNT}"; then
    echo "Error: '${MOUNT}' is already mounted"
    exit 1
fi
# Create mount point if it doesn't exist
mkdir -p "${MOUNT}"                         
##### Mount subvolumes #####
mount -o subvol=/,user_subvol_rm_allowed "${DEVICE}" "${MOUNT}"
##### Validate CLEAN_ROOT exists #####
if [ -z "$CLEAN_ROOT" ]; then
  echo "Usage: $0 ${CLEAN_ROOT} does not exist"
  echo "Example: $0 /mnt/@root /mnt/@snapshots CLEAN_ROOT '/etc/nixos /home/alexanderschiffhauer'"
  exit 1
fi

############################
##### Manage snapshots #####
############################
##### Create Snapshot path variables #####
PREVIOUS_SNAPSHOT="${SNAPSHOTS}/PREVIOUS_SNAPSHOT"        # /mnt/@snapshots/PREVIOUS_SNAPSHOT
PENULTIMATE_SNAPSHOT="${SNAPSHOTS}/PENULTIMATE_SNAPSHOT"  # /mnt/@snapshots/PENULTIMATE_SNAPSHOT
CURRENT_SNAPSHOT="${SNAPSHOTS}/CURRENT_SNAPSHOT"          # /mnt/@snapshots/CURRENT_SNAPSHOT
##### If it exists, delete the penultimate snapshot. #####
[ -d "${PENULTIMATE_SNAPSHOT}" ] && btrfs subvolume delete -R "${PENULTIMATE_SNAPSHOT}"
##### If a previous snapshot exists, make it the penultimate snapshot and delete it. #####
[ -d "${PREVIOUS_SNAPSHOT}" ] && btrfs subvolume snapshot "${PREVIOUS_SNAPSHOT}" "${PENULTIMATE_SNAPSHOT}" && btrfs subvolume delete -R "${PREVIOUS_SNAPSHOT}"  
##### Make a previous snapshot capturing the current state of the system. #####
btrfs subvolume snapshot "${ROOT}" "${PREVIOUS_SNAPSHOT}"
##### Create the new (i.e. current) snapshot, copied from a known clean copy of the system. #####
echo "Creating a clean system image, '$CURRENT_SNAPSHOT', from '$CLEAN_ROOT'..."
btrfs subvolume snapshot "$CLEAN_ROOT" "$CURRENT_SNAPSHOT"
##### Make the current snapshot read-writeable #####
btrfs property set -ts "$CURRENT_SNAPSHOT" ro false 2>/dev/null || true

######################
##### Copy paths #####
######################
##### Copy the explicitly preserved paths into the current snapshot. #####
echo "Copying '${PATHS_TO_KEEP}' to '${CURRENT_SNAPSHOT}'..."
for path in $PATHS_TO_KEEP; do
  current_path="${ROOT}/${path}"
  tmp_path="${CURRENT_SNAPSHOT}/${path}"
  if [ -e "${current_path}" ]; then
    mkdir -p "$(dirname "${tmp_path}")"
    rsync -avxHAX "${current_path}" "${tmp_path}" # rsync -avxHAX should preserve everything
    echo "Successfully copied '${current_path}' to '${tmp_path}'"
  else
    echo "Warning: '${current_path}' does not exist and was not preserved."
  fi
done
##### NixOS depends on an unfathomable amount of symlinks, so just copy all of them into the current snapshot. #####
echo "Preserving new symlinks..."
(
  cd "${ROOT}"
  find . -type l | while read -r link; do
    if [ ! -e "${CURRENT_SNAPSHOT}/${link}" ]; then
      mkdir -p "$(dirname "${CURRENT_SNAPSHOT}/${link}")"
      cp -a -H "${link}" "${CURRENT_SNAPSHOT}/${link}"
      echo "Successfully copied '${link}' to '${CURRENT_SNAPSHOT}/${link}'"
    fi
  done
)

#####################
##### Swap root #####
#####################
##### Delete root subvolume (!) #####
##### If the power is plugged now, you can restore /mnt/@snapshots/PREVIOUS_SNAPSHOT #####
echo "Deleting '${ROOT}'..."
btrfs subvolume delete -R "${ROOT}"
##### Re-create the root subvolume by creating a snapshot based on what we just constructed. #####
echo "Restoring '${ROOT}'..."
btrfs subvolume snapshot "${CURRENT_SNAPSHOT}" "${ROOT}"
##### Unmount #####
umount "${MOUNT}"
##### Finish #####
echo "Done. '${ROOT}' has been restored to the state of '${CLEAN_ROOT}' + new symlinks + keep-paths."
