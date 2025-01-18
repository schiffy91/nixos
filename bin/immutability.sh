set -euo pipefail
log_info() {
	echo "MSG $@"
}
log_warning() {
	echo "WRN $@" >&2
}
log_error() {
	echo "ERR $@" >&2
}
trace() {
	log_info "$@"
	"$@"
	local ret=$?
	if [ ! $ret -eq 0 ]; then
		log_warning "$@ returned with status code $ret" >&2
	fi
	return $ret
}
abort() {
	echo ""
	log_error "$@"
	log_error "Unmounting and quitting"
	echo ""
	trace btrfs_sync "$ROOT"
	trace disk_unmount "$MOUNT"
	exit 1
}
disk_mount() {
	trace mkdir -p "$2"
	#trace mount -t btrfs -o subvolid=5,user_subvol_rm_allowed "$1" "$2"
}
disk_unmount() {
	#trace umount "$1"
	#trace rm -rf "$1"
	echo
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
	trace btrfs subvolume delete "$path" || abort "Failed to delete $path"
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

log_info "Setting up variables"
MOUNT="/mnt"
DEVICE="$1"                 # /dev/disk/by-label/disk-main-root
ROOT="$MOUNT/$2"            # /mnt/@root <--------------------  @root
SNAPSHOTS="$MOUNT/$3"       # /mnt/@snapshots <---------------  @snapshots
CLEAN_ROOT="$SNAPSHOTS/$4"  # /mnt/@snapshots/CLEAN_ROOT <---- CLEAN_ROOT
PATHS_TO_KEEP="$5"          # "/etc/nixos /etc/machine-id /home/alexanderschiffhauer"

log_info "MOUNT=$MOUNT ROOT=$ROOT SNAPSHOTS=$SNAPSHOTS CLEAN_ROOT=$CLEAN_ROOT PATHS_TO_KEEP=$PATHS_TO_KEEP"
trace require "-b $DEVICE"
trace require "-n $CLEAN_ROOT"
trace require "-d $CLEAN_ROOT"
trace disk_mount "$DEVICE" "$MOUNT"
PREVIOUS_SNAPSHOT="$SNAPSHOTS/PREVIOUS_SNAPSHOT"              # /mnt/@snapshots/PREVIOUS_SNAPSHOT
PENULTIMATE_SNAPSHOT="$SNAPSHOTS/PENULTIMATE_SNAPSHOT"        # /mnt/@snapshots/PENULTIMATE_SNAPSHOT
CURRENT_SNAPSHOT="$SNAPSHOTS/CURRENT_SNAPSHOT"                # /mnt/@snapshots/CURRENT_SNAPSHOT
trace desire -d "$PENULTIMATE_SNAPSHOT" || trace btrfs_subvolume_copy "$CLEAN_ROOT" "$PENULTIMATE_SNAPSHOT"
trace desire -d "$PREVIOUS_SNAPSHOT" || trace btrfs_subvolume_copy "$CLEAN_ROOT" "$PREVIOUS_SNAPSHOT"

log_info "Setting up snapshots"
trace btrfs_subvolume_copy "$PREVIOUS_SNAPSHOT" "$PENULTIMATE_SNAPSHOT"
trace btrfs_subvolume_copy "$ROOT" "$PREVIOUS_SNAPSHOT"
trace btrfs_subvolume_copy "$CLEAN_ROOT" "$CURRENT_SNAPSHOT"
trace btrfs_subvolume_rw "$CURRENT_SNAPSHOT"

log_info "Copying $CURRENT_SNAPSHOT to $ROOT"
#trace btrfs_subvolume_copy "$CURRENT_SNAPSHOT" "$ROOT"
trace disk_unmount "$MOUNT"