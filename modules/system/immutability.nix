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
    #kernelModules = initrdKernelModules;
    #availableKernelModules = initrdKernelModules;
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
      INDENT_DEPTH=0
      indent() {
        local tabs=""
        for ((i=1; i<=INDENT_DEPTH; i++)); do
          tabs+="\t"
        done
        echo "$tabs"
      }
      log_info() {
        echo "$(indent)INFO    : $@"
      }
      log_error() {
        echo "$(indent)ERROR   : $@" >&2
      }
      trace() {
        ((INDENT_DEPTH++))
        echo "$(indent)STARTED : $@"
        "$@"
        local ret=$?
        if [ $ret -eq 0 ]; then
          echo "$(indent)FINISHED: $@"
        else
          echo "$(indent)FAILED  : $@" >&2
        fi
        ((INDENT_DEPTH--))
        return $ret
      }
      warn() {
        log_error "$@"
      }
      abort() {
        warn "$@"
        trace disk_unmount "$MOUNT"
        exit 1
      }
      disk_mount() {
        trace mkdir -p "$2"
        trace mount -t btrfs -o subvolid=5,user_subvol_rm_allowed "$1" "$2"
      }
      disk_unmount() {
        trace umount "$1"
        trace rm -rf "$1"
      }
      require() {
        trace test "$@" || abort "Require failed: $@"
      }
      desire() {
        trace test "$@" && return 0
        warn "Desire failed: $@"
        return 1
      }
      btrfs_sync() {
        local path="$1"
        trace btrfs filesystem sync "$path"
      }
      btrfs_subvolume_delete() {
        trace btrfs subvolume delete "$@"
        trace btrfs_sync "$ROOT"
      }
      btrfs_subvolume_delete_recursively() {
        local path="$1"
        [ -d "$path" ] || (warn "$path is not a directory" && return 0)
        local subvolumes
        subvolumes=$(btrfs subvolume list -o "$path" | tac | cut -f 9- -d ' ')
        IFS=$'\n'
        for subvolume in $subvolumes; do
          trace btrfs_subvolume_delete "$MOUNT/$subvolume"
        done
        trace btrfs_subvolume_delete "$path" 
      }
      btrfs_subvolume_copy() {
        local source="$1"
        local target="$2"
        desire "-d $source" || return 1
        trace btrfs_subvolume_delete_recursively "$target" || return 1
        trace btrfs subvolume snapshot "$source" "$target" || abort "Failed to create snapshot from $source to $target"
        trace btrfs_sync "$ROOT"
      }
      
      log_info "Setting up variables"
      MOUNT="/mnt"
      DEVICE="$1"                 # /dev/disk/by-label/disk-main-root
      ROOT="$MOUNT/$2"            # /mnt/@root <--------------------  @root
      SNAPSHOTS="$MOUNT/$3"       # /mnt/@snapshots <---------------  @snapshots
      CLEAN_ROOT="$SNAPSHOTS/$4"  # /mnt/@snapshots/CLEAN_ROOT <---- CLEAN_ROOT
      PATHS_TO_KEEP="$5"          # "/etc/nixos /etc/machine-id /home/alexanderschiffhauer"
      
      log_info "MOUNT=$MOUNT ROOT=$ROOT SNAPSHOTS=$SNAPSHOTS CLEAN_ROOT=$CLEAN_ROOT PATHS_TO_KEEP=$PATHS_TO_KEEP"
      require "-b $DEVICE"
      require "-n $CLEAN_ROOT"
      require "-d $CLEAN_ROOT"
      trace disk_mount "$DEVICE" "$MOUNT"
      PREVIOUS_SNAPSHOT="$SNAPSHOTS/PREVIOUS_SNAPSHOT"              # /mnt/@snapshots/PREVIOUS_SNAPSHOT
      PENULTIMATE_SNAPSHOT="$SNAPSHOTS/PENULTIMATE_SNAPSHOT"        # /mnt/@snapshots/PENULTIMATE_SNAPSHOT
      CURRENT_SNAPSHOT="$SNAPSHOTS/CURRENT_SNAPSHOT"                # /mnt/@snapshots/CURRENT_SNAPSHOT
      desire -d "$PENULTIMATE_SNAPSHOT" || trace btrfs_subvolume_copy "$CLEAN_ROOT" "$PENULTIMATE_SNAPSHOT"
      desire -d "$PREVIOUS_SNAPSHOT" || trace btrfs_subvolume_copy "$CLEAN_ROOT" "$PREVIOUS_SNAPSHOT"
      
      log_info "Setting up snapshots"
      trace btrfs_subvolume_copy "$PREVIOUS_SNAPSHOT" "$PENULTIMATE_SNAPSHOT"
      trace btrfs_subvolume_copy "$ROOT" "$PREVIOUS_SNAPSHOT"
      trace btrfs_subvolume_copy "$CLEAN_ROOT" "$CURRENT_SNAPSHOT"
      trace btrfs property set -ts "$CURRENT_SNAPSHOT" ro false 2>/dev/null || true # readwrite
      
      log_info "Copying $CURRENT_SNAPSHOT to $ROOT"
      trace btrfs_subvolume_copy "$CURRENT_SNAPSHOT" "$ROOT"
      trace disk_unmount "$MOUNT"
      '';
    };
  };
}