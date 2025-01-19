set -euo pipefail
LOG_DEPTH=0
LOG_SPACES_PER_LEVEL=2
indent_log() {
	spaces=$((LOG_DEPTH * LOG_SPACES_PER_LEVEL))
	i=0
	while [ $i -lt $spaces ]; do
		echo -n " "
		i=$((i + 1))
	done
}
log() {
	indent_log
	echo "$@"
}
log_info() {
	log "MSG $@"
}
log_warning() {
	log "WRN $@" >&2
}
log_error() {
	log "ERR $@" >&2
}
trace() {
	LOG_DEPTH=$((LOG_DEPTH + 1))
	log_info "$@"
	"$@"
	local ret=$?
	if [ ! $ret -eq 0 ]; then
		log_warning "$@ returned with status code $ret" >&2
	fi
	LOG_DEPTH=$((LOG_DEPTH - 1))
	return $ret
}
abort() {
	echo ""
	log_error "$@"
	log_error "Unmounting and quitting"
	echo ""
	trace btrfs_sync "$EPHEMERAL_SUBVOLUME"
	trace subvolumes_unmount "$MOUNT_POINT"
	exit 1
}
subvolumes_mount() {
		local device="$1"
		local mount="$2"
		shift 2

		while [ $# -ge 1 ]; do
			local subvolume_name="$1"
			trace mkdir -p "$mount/$subvolume_name"
			trace mount -t btrfs -o subvol="$subvolume_name",user_subvol_rm_allowed "$device" "$mount/$subvolume_name"
			shift 1
		done
}
subvolumes_unmount() {
	trace umount -R "$1"
	trace rm -rf "$1"
}
require() {
	trace test "$@" || abort "Require failed: $@"
}
desire() {
	trace test "$@" || return 1
}
btrfs_sync() {
	local path="$1"
	trace btrfs filesystem sync "$path"
}
btrfs_subvolume_delete() {
	local path="$1"
	trace btrfs subvolume delete "$path" --commit-after || abort "Failed to delete $path"
	trace btrfs_sync "$(dirname $path)"
}
btrfs_subvolume_delete_recursively() {
	local path="$1"
	trace desire -d "$path" || return 0
	local subvolumes
	subvolumes=$(btrfs subvolume list -o "$path" | tac | cut -f 9- -d ' ')
	IFS=$'\n'
	for subvolume in $subvolumes; do
		trace btrfs_subvolume_delete "$path/$subvolume"
	done
	trace btrfs_subvolume_delete "$path"
}
btrfs_subvolume_copy() {
	local source="$1"
	local target="$2"
	trace require "-d $source"
	trace btrfs_subvolume_delete_recursively "$target"
	trace btrfs subvolume snapshot "$source" "$target" || abort "Failed to create snapshot from $source to $target"
	trace btrfs_sync "$source"
}
btrfs_subvolume_rw() {
	local path="$1"
	trace btrfs property set -ts "$path" ro false 2>/dev/null || abort "Failed to make $path read-write"
}
btrfs_verify() {
	local device="$1"
	trace require "[ \"$(blkid -p "$device" | grep 'TYPE=' | grep -o 'btrfs')\" = 'btrfs' ]" || abort "Device $device is not a btrfs filesystem"
}

log_info "Setting up variables"
MOUNT_POINT="/mnt"
DISK="$1"																																# /dev/disk/by-label/disk-main-root
EPHEMERAL_SUBVOLUME_NAME="$2"																						# @root
EPHEMERAL_SUBVOLUME="$MOUNT_POINT/$EPHEMERAL_SUBVOLUME_NAME"						# /mnt/@root
SNAPSHOTS_SUBVOLUME_NAME="$3"																						# @snapshots
SNAPSHOTS_SUBVOLUME="$MOUNT_POINT/$SNAPSHOTS_SUBVOLUME_NAME"						# /mnt/@snapshots
PATH_TO_CLEAN_SNAPSHOT="$SNAPSHOTS_SUBVOLUME/$4"  											# /mnt/@snapshots/PATH_TO_CLEAN_SNAPSHOT <---- PATH_TO_CLEAN_SNAPSHOT
PATHS_TO_KEEP="$5"          																						# "/etc/nixos /etc/machine-id /home/alexanderschiffhauer"

log_info "MOUNT_POINT=$MOUNT_POINT EPHEMERAL_SUBVOLUME=$EPHEMERAL_SUBVOLUME SNAPSHOTS_SUBVOLUME=$SNAPSHOTS_SUBVOLUME PATH_TO_CLEAN_SNAPSHOT=$PATH_TO_CLEAN_SNAPSHOT PATHS_TO_KEEP=$PATHS_TO_KEEP"
trace require [ "$#" -eq 5 ] || abort "Usage: $0 <disk> <ephemeral_subvolume> <snapshots_subvolume> <clean_snapshot> <paths_to_keep>"
trace btrfs_verify "$DISK"
trace require "-b $DISK"
trace require "-n $PATH_TO_CLEAN_SNAPSHOT"
trace require "-d $PATH_TO_CLEAN_SNAPSHOT"
trace subvolumes_mount "$DISK" "$MOUNT_POINT" "$EPHEMERAL_SUBVOLUME_NAME" "$SNAPSHOTS_SUBVOLUME_NAME"
PREVIOUS_SNAPSHOT="$SNAPSHOTS_SUBVOLUME/PREVIOUS_SNAPSHOT"              # /mnt/@snapshots/PREVIOUS_SNAPSHOT
PENULTIMATE_SNAPSHOT="$SNAPSHOTS_SUBVOLUME/PENULTIMATE_SNAPSHOT"        # /mnt/@snapshots/PENULTIMATE_SNAPSHOT
CURRENT_SNAPSHOT="$SNAPSHOTS_SUBVOLUME/CURRENT_SNAPSHOT"                # /mnt/@snapshots/CURRENT_SNAPSHOT
trace desire -d "$PENULTIMATE_SNAPSHOT" || trace btrfs_subvolume_copy "$PATH_TO_CLEAN_SNAPSHOT" "$PENULTIMATE_SNAPSHOT"
trace desire -d "$PREVIOUS_SNAPSHOT" || trace btrfs_subvolume_copy "$PATH_TO_CLEAN_SNAPSHOT" "$PREVIOUS_SNAPSHOT"

log_info "Setting up snapshots"
trace btrfs_subvolume_copy "$PREVIOUS_SNAPSHOT" "$PENULTIMATE_SNAPSHOT"
trace btrfs_subvolume_copy "$EPHEMERAL_SUBVOLUME" "$PREVIOUS_SNAPSHOT"
trace btrfs_subvolume_copy "$PATH_TO_CLEAN_SNAPSHOT" "$CURRENT_SNAPSHOT"
trace btrfs_subvolume_rw "$CURRENT_SNAPSHOT"

#TODO Preserve persistent paths
#TODO Preserve all symlinks

log_info "Copying $CURRENT_SNAPSHOT to $EPHEMERAL_SUBVOLUME"
trace btrfs_subvolume_copy "$CURRENT_SNAPSHOT" "$EPHEMERAL_SUBVOLUME"
trace subvolumes_unmount "$MOUNT_POINT"