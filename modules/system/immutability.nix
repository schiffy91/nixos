{ config, lib, pkgs, ... }: 
let 
	device = config.settings.disk.by.partlabel.root;
	snapshotsSubvolumeName = config.settings.disk.subvolumes.snapshots.name;
	cleanName = config.settings.disk.immutability.persist.snapshots.cleanName;
	pathsToKeep = "\"${lib.strings.concatStringsSep " " config.settings.disk.immutability.persist.paths}\"";
	subvolumeNameMountPointPairs = "\"${config.settings.disk.subvolumes.nameMountPointPairs.resetOnBoot}\"";
	rootDevice = "dev-disk-by\\x2dpartlabel-${config.settings.disk.label.disk}\\x2d${config.settings.disk.label.main}\\x2d${config.settings.disk.label.root}.device"; # JFC…
in 
lib.mkIf config.settings.disk.immutability.enable {
	fileSystems = lib.mkMerge (lib.lists.forEach (lib.filter (volume: volume.neededForBoot) config.settings.disk.subvolumes.volumes) (volume: { "${volume.mountPoint}".neededForBoot = lib.mkForce true; }));
	boot.readOnlyNixStore = true;
	boot.tmp.useTmpfs = true;
	boot.initrd = {
		supportedFilesystems = [ "btrfs" ];
		systemd = {
			extraBin = {
				rsync = "${pkgs.rsync}/bin/rsync";
			};
			services.immutability = {
				description = "Apply immutability on-boot by resetting the filesystem to the original BTRFS snapshot and copying symlinks and intentionally preserved files";
				wantedBy = [ "initrd.target" ];
				requires = [ rootDevice ];
				after = [ rootDevice "systemd-cryptsetup@${config.settings.disk.partlabel.root}.service" ];
				before = [ "sysroot.mount" ];
				unitConfig.DefaultDependencies = "no";
				serviceConfig.Type = "oneshot";
				scriptArgs = "${device} ${snapshotsSubvolumeName} ${cleanName} ${subvolumeNameMountPointPairs} ${pathsToKeep}";
				script = ''
					LOG_DEPTH=0
					log() {
						local joined="$(echo "$*" | tr '\n' ' ')"
						indentations=$((LOG_DEPTH * 2))
						i=0
						while [ $i -lt $indentations ]; do
							echo -n " "
							i=$((i + 1))
						done
						echo "$joined"
					}
					log_warning() {
						log "WRN $@" >&2
					}
					log_error() {
						log "ERR $@" >&2
					}
					trace() {
						LOG_DEPTH=$((LOG_DEPTH + 1))
						log "$*"
						local output
						output=$("$@" 2>&1)
						local ret=$?
						while IFS= read -r line; do
							[ -n "$line" ] && log "$line"
						done <<< "$output"
						if [ $ret -ne 0 ]; then
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
						trace btrfs_sync "$SUBVOLUME"
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
						for subvolume_name in $subvolume_names $snapshots_subvolume_name; do
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
						trace test -d "$path" || return 0
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
						trace btrfs_sync "$target"
					}
					btrfs_subvolume_rw() {
						local path="$1"
						trace btrfs property set -ts "$path" ro false || abort "Failed to make $path read-write"
					}
					files_copy() {
						local subvolume_mount_point="$1"
						local paths_to_keep="$2"
						local previous_snapshot="$3"
						local current_snapshot="$4"
						(
							local rsync_paths=( --include="*/" )
							for path_to_keep in $paths_to_keep; do
								if [[ "$path_to_keep" == "$subvolume_mount_point"* ]]; then
									local path_in_snapshot="''${path_to_keep#$subvolume_mount_point}"
									path_in_snapshot="''${path_in_snapshot#/}"
									rsync_paths+=( --include="$path_in_snapshot" )
								fi
							done
							rsync_paths+=( --exclude="*" )
							if [ "''${#rsync_paths[@]}" -gt 2 ]; then
								trace cd "$previous_snapshot"
								trace rsync -aHAX --numeric-ids --delete "''${rsync_paths[@]}" . "$current_snapshot"
							else
								log "No paths matched $subvolume_mount_point; skipping files_copy for this subvolume."
							fi
						)
					}
					log "Setting up variables"
					MOUNT_POINT="/mnt"
					DISK="$1"
					SNAPSHOTS_SUBVOLUME_NAME="$2"
					CLEAN_SNAPSHOT_NAME="$3"
					SUBVOLUME_NAME_MOUNT_POINT_PAIRS="$4"
					PATHS_TO_KEEP=$(echo "$5" | tr ' ' '\n' | sort)
					SUBVOLUME_NAMES=""
					for pair in $SUBVOLUME_NAME_MOUNT_POINT_PAIRS; do
						subvolume_name=''${pair%=*}
						if [ -z "$SUBVOLUME_NAMES" ]; then
							SUBVOLUME_NAMES="$subvolume_name"
						else
							SUBVOLUME_NAMES="$SUBVOLUME_NAMES $subvolume_name"
						fi
					done
					PREVIOUS_SNAPSHOT_NAME="PREVIOUS"
					PENULTIMATE_SNAPSHOT_NAME="PENULTIMATE"
					CURRENT_SNAPSHOT_NAME="CURRENT"
					log "MOUNT_POINT=$MOUNT_POINT SNAPSHOTS_SUBVOLUME_NAME=$SNAPSHOTS_SUBVOLUME_NAME SUBVOLUME_NAMES=$SUBVOLUME_NAMES PATHS_TO_KEEP=$PATHS_TO_KEEP"
					
					log "Mounting $DISK, $SNAPSHOTS_SUBVOLUME_NAME, and $SUBVOLUME_NAMES"
					trace subvolumes_mount "$DISK" "$MOUNT_POINT" "$SNAPSHOTS_SUBVOLUME_NAME" "$SUBVOLUME_NAMES"
					
					for pair in $SUBVOLUME_NAME_MOUNT_POINT_PAIRS; do
						SUBVOLUME_NAME=''${pair%=*}
						SUBVOLUME_MOUNT_POINT=''${pair#*=}
						log "Resetting $SUBVOLUME_NAME"
						SUBVOLUME="$MOUNT_POINT/$SUBVOLUME_NAME"
						SNAPSHOTS="$MOUNT_POINT/$SNAPSHOTS_SUBVOLUME_NAME/$SUBVOLUME_NAME"
						CLEAN_SNAPSHOT="$SNAPSHOTS/$CLEAN_SNAPSHOT_NAME"
						PREVIOUS_SNAPSHOT="$SNAPSHOTS/$PREVIOUS_SNAPSHOT_NAME"
						PENULTIMATE_SNAPSHOT="$SNAPSHOTS/$PENULTIMATE_SNAPSHOT_NAME"
						CURRENT_SNAPSHOT="$SNAPSHOTS/$CURRENT_SNAPSHOT_NAME"

						log "Validating $CLEAN_SNAPSHOT, and initializing $PENULTIMATE_SNAPSHOT and $PREVIOUS_SNAPSHOT if they don't exist."
						trace require -n "$CLEAN_SNAPSHOT" && trace require -d "$CLEAN_SNAPSHOT"
						trace test -d "$PENULTIMATE_SNAPSHOT" || trace btrfs_subvolume_copy "$CLEAN_SNAPSHOT" "$PENULTIMATE_SNAPSHOT"
						trace test -d "$PREVIOUS_SNAPSHOT" || trace btrfs_subvolume_copy "$CLEAN_SNAPSHOT" "$PREVIOUS_SNAPSHOT"

						log "Setting $PENULTIMATE_SNAPSHOT, $PREVIOUS_SNAPSHOT, and $CURRENT_SNAPSHOT"
						trace btrfs_subvolume_copy "$PREVIOUS_SNAPSHOT" "$PENULTIMATE_SNAPSHOT"
						trace btrfs_subvolume_copy "$SUBVOLUME" "$PREVIOUS_SNAPSHOT"
						trace btrfs_subvolume_copy "$CLEAN_SNAPSHOT" "$CURRENT_SNAPSHOT"
						trace btrfs_subvolume_rw "$CURRENT_SNAPSHOT"

						log "Preserving persistent paths from $PREVIOUS_SNAPSHOT into $CURRENT_SNAPSHOT"
						trace files_copy "$SUBVOLUME_MOUNT_POINT" "$PATHS_TO_KEEP" "$PREVIOUS_SNAPSHOT" "$CURRENT_SNAPSHOT"

						log_warning "TODO: Preserve new symlinks from $PREVIOUS_SNAPSHOT into $CURRENT_SNAPSHOT"

						log "Copying $CURRENT_SNAPSHOT to $SUBVOLUME"
						log_warning "TODO: Make this operation atomic by using btrfs subvolume set-default <new_path> <old_path> on new TMP subvolumes"
						trace btrfs_subvolume_copy "$CURRENT_SNAPSHOT" "$SUBVOLUME"
					done
					trace subvolumes_unmount "$MOUNT_POINT"
				'';
			};
		};
	};
}