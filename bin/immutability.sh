#!/usr/bin/env bash
#
# restore-subvolume.sh - revert a Btrfs subvolume from a snapshot,
#                        preserving user-specified paths and ignoring new symlinks.
#
# Usage:
#   restore-subvolume.sh <subvolumePath> <snapshotPath> "<keepPaths>"
#
# Example:
#   restore-subvolume.sh /mnt/@root /mnt/@root_snapshots/FACTORY_RESET "/etc /home/mydir"
#
set -euo pipefail

# Parse args
SUBVOLUME="${1:-}"                # e.g. "/mnt/@root"
NEW_SUBVOLUME="${SUBVOLUME}.new"  # e.g. /mnt/@root.new
SNAPSHOT="${2:-}"                 # e.g. "/snapshots/@root/FACTORY_RESET"
KEEP_PATHS="${3:-}"               # e.g. "/etc/nixos /etc/machine-id /home/alexanderschiffhauer"
if [ -z "$SUBVOLUME" ] || [ -z "$SNAPSHOT" ]; then
  echo "Usage: $0 <subvolume> <snapshot> <keep-paths>"
  echo "Example: $0 /mnt/@root /snapshots/@root/FACTORY_RESET '/etc/nixos /etc/machine-id'"
  exit 1
fi

# 1) Clean dangling new subvolumes (i.e. poweroutage previous during boot)
if [ -d "$NEW_SUBVOLUME" ]; then
  echo "Removing leftover '$NEW_SUBVOLUME' from previous failed run..."
  btrfs subvolume delete "$NEW_SUBVOLUME"
fi

# 2) Create a new (writable) snapshot from the original snapshot
echo "Creating new snapshot '$NEW_SUBVOLUME' from '$SNAPSHOT'..."
btrfs subvolume snapshot "$SNAPSHOT" "$NEW_SUBVOLUME"
btrfs property set -ts "$NEW_SUBVOLUME" ro false 2>/dev/null || true

# 3) Copy “KEEP_PATHS” from the current (old) subvolume into the new subvolume
echo "Copying user-specified paths to the new subvolume..."
for path in $KEEP_PATHS; do
  oldPath="$SUBVOLUME/$path"
  newPath="$NEW_SUBVOLUME/$path"
  if [ -e "$oldPath" ]; then
    mkdir -p "$(dirname "$newPath")"
    rsync -a "$oldPath" "$newPath"
  fi
done

# 4) Copy newly introduced symlinks that do not exist in the snapshot
echo "Preserving new symlinks..."
(
  cd "$SUBVOLUME"
  find . -type l | while read -r link; do
    if [ ! -e "$NEW_SUBVOLUME/$link" ]; then
      mkdir -p "$(dirname "$NEW_SUBVOLUME/$link")"
      cp -a "$link" "$NEW_SUBVOLUME/$link"
    fi
  done
)

# 5) Use 'mv' to merge-rename the new subvolume on top of the old one
echo "Merging '$NEW_SUBVOLUME' onto '$SUBVOLUME'..."
mv "$NEW_SUBVOLUME" "$SUBVOLUME"

# 6) Now remove the “old” subvolume that ended up as a subfolder
#    Named exactly the same as SUBVOLUME’s basename, e.g. if SUBVOLUME="/mnt/@root",
#    then we remove "/mnt/@root/@root".
oldSub="${SUBVOLUME}/$(basename "$SUBVOLUME")"
echo "Removing old subvolume '$oldSub'..."
if [ -d "$oldSub" ]; then
  btrfs subvolume delete "$oldSub"
fi

echo "Done. '$SUBVOLUME' has been restored to the state of '$SNAPSHOT' + new symlinks + keep-paths."
