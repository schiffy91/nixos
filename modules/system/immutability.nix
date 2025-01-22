{ config, lib, pkgs, ... }: 
let 
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
	boot.tmp.useTmpfs = true;
	boot.initrd = {
		supportedFilesystems = [ "btrfs" ];
		systemd.services.immutability = {
			description = "Apply immutability on-boot by resetting the filesystem to the original BTRFS snapshot and copying symlinks and intentionally preserved files";
			wantedBy = [ "initrd.target" ];
			requires = [ rootDevice ];
			after = [ rootDevice "systemd-cryptsetup@${config.settings.disk.partlabel.root}.service" ];
			before = [ "sysroot.mount" ];
			unitConfig.DefaultDependencies = "no";
			serviceConfig.Type = "oneshot";
			scriptArgs = "${device} ${rootSubvolumeName} ${snapshotsSubvolumeName} ${cleanRootSnapshotRelativePath} ${pathsToKeep}";
			script = ''
				set -euo pipefail
				LOG_DEPTH=0
				LOG_SPACES_PER_LEVEL=2
				indent() {
					spaces=$((LOG_DEPTH * LOG_SPACES_PER_LEVEL))
					i=0
					while [ $i -lt $spaces ]; do
						echo -n " "
						i=$((i + 1))
					done
				}
				log() {
					indent
					echo "$@"
				}
				log_warning() {
					log "WRN $@" >&2
				}
				log_error() {
					log "ERR $@" >&2
				}
				trace() {
					LOG_DEPTH=$((LOG_DEPTH + 1))
					log "$@"
					output=$("$@" 2>&1)
					local ret=$?
					echo "$output" | while read -r line; do
						[ -n "$line" ] && log "$line"
					done
					if [ ! $ret -eq 0 ]; then
						log_warning "$@ returned with status code $ret"
					fi
					LOG_DEPTH=$((LOG_DEPTH - 1))
					return $ret
				}
				abort() {
					echo ""
					log_error "$@"
					log_error "Unmounting and quitting"
					echo ""
					trace btrfs_sync "$SUBVOUME"
					trace subvolumes_unmount "$MOUNT_POINT"
					exit 1
				}
				subvolumes_mount() {
					local disk="$1"
					local mount_point="$2"
					local snapshots_subvolume_name="$3"
					local subvolume_names="$4"

					trace require "-b $disk"
					trace mkdir -p "$mount_point"

					trace mount -t btrfs -o subvolid=5,user_subvol_rm_allowed "$disk" "$mount_point"
					for subvolume_name in "$subvolume_names $snapshots_subvolume_name"; do
						trace mkdir -p "$mount_point/$subvolume_name"
						trace mount -t btrfs -o subvol="$subvolume_name",user_subvol_rm_allowed "$disk" "$mount_point/$subvolume_name"
					done
				}
				subvolumes_unmount() {
					local mount_point="$1"
					trace umount -R "$mount_point"
					trace rm -rf "$mount_point"
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
					subvolumes=$(btrfs subvolume list -o "$path" | cut -f 9- -d ' ')
					IFS=$'\n'
					for subvolume in $subvolumes; do
						trace btrfs_subvolume_delete_recursively "$(dirname $path)/$subvolume"
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
					trace btrfs property set -ts "$path" ro false || abort "Failed to make $path read-write"
				}

				log "Setting up variables"
				MOUNT_POINT="/mnt"
				DISK="$1"
				SNAPSHOTS_SUBVOLUME_NAME="$2"
				SUBVOLUME_NAMES="$3"
				PATHS_TO_KEEP="$4"
				CLEAN_SNAPSHOT_NAME="CLEAN"
				PREVIOUS_SNAPSHOT_NAME="REVIOUS"
				PENULTIMATE_SNAPSHOT_NAME="PENULTIMATE"
				CURRENT_SNAPSHOT_NAME="CURRENT"
				log "MOUNT_POINT=$MOUNT_POINT SNAPSHOTS_SUBVOLUME_NAME=$SNAPSHOTS_SUBVOLUME_NAME SUBVOLUME_NAMES=$SUBVOLUME_NAMES PATHS_TO_KEEP=$PATHS_TO_KEEP"
				
				log "Mounting $DISK, $SNAPSHOTS_SUBVOLUME_NAME, and $SUBVOLUME_NAMES"
				trace subvolumes_mount "$DISK" "$MOUNT_POINT" "$SNAPSHOTS_SUBVOLUME_NAME" "$SUBVOLUME_NAMES"
				
				for SUBVOUME_NAME in $SUBVOLUME_NAMES; do
					log "Resetting $SUBVOLUME_NAME"
					SUBVOUME="$MOUNT_POINT/$SUBVOUME_NAME"
					SNAPSHOTS="$MOUNT_POINT/$SNAPSHOTS_SUBVOLUME_NAME/$SUBVOUME_NAME"
					CLEAN_SNAPSHOT="$SNAPSHOTS/$CLEAN_SNAPSHOT_NAME"
					PREVIOUS_SNAPSHOT="$SNAPSHOTS/$PREVIOUS_SNAPSHOT_NAME"
					PENULTIMATE_SNAPSHOT="$SNAPSHOTS/$PENULTIMATE_SNAPSHOT_NAME"
					CURRENT_SNAPSHOT="$SNAPSHOTS/$CURRENT_SNAPSHOT_NAME"

					log "Validating $CLEAN_SNAPSHOT, and initializing $PENULTIMATE_SNAPSHOT and $PREVIOUS_SNAPSHOT if they don't exist."
					trace require "-n $CLEAN_SNAPSHOT" && trace require "-d $CLEAN_SNAPSHOT"
					trace desire -d "$PENULTIMATE_SNAPSHOT" || trace btrfs_subvolume_copy "$CLEAN_SNAPSHOT" "$PENULTIMATE_SNAPSHOT"
					trace desire -d "$PREVIOUS_SNAPSHOT" || trace btrfs_subvolume_copy "$CLEAN_SNAPSHOT" "$PREVIOUS_SNAPSHOT"

					log "Setting $PENULTIMATE_SNAPSHOT, $PREVIOUS_SNAPSHOT, and $CURRENT_SNAPSHOT"
					trace btrfs_subvolume_copy "$PREVIOUS_SNAPSHOT" "$PENULTIMATE_SNAPSHOT"
					trace btrfs_subvolume_copy "$SUBVOUME" "$PREVIOUS_SNAPSHOT"
					trace btrfs_subvolume_copy "$CLEAN_SNAPSHOT" "$CURRENT_SNAPSHOT"
					trace btrfs_subvolume_rw "$CURRENT_SNAPSHOT"

					#TODO Preserve persistent paths from PREVIOUS_SNAPSHOT into CURRENT_SNAPSHOT
					#TODO Preserve new symlinks from PREVIOUS_SNAPSHOT into CURRENT_SNAPSHOT

					log "Copying $CURRENT_SNAPSHOT to $SUBVOUME"
					#TODO Use btrfs subvolume set-default <new_path> <old_path> on SUBVOLUME_TMP, then delete and replace SUBVOLUME and last use btrfs subvolume set-default again.
					trace btrfs_subvolume_copy "$CURRENT_SNAPSHOT" "$SUBVOUME"
				done
				trace subvolumes_unmount "$MOUNT_POINT"
			'';
		};
	};
}