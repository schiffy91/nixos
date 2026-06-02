#!/usr/bin/env bash
# migrate-persist-to-root.sh
#
# One-shot migration: copy content from @persist/dirs/<key> back into the
# underlying @root and @home subvolumes, so a future boot with immutability
# disabled sees the same /etc/nixos, /home/<user>, /var/lib/<...> content
# that the currently-running (immutability-enabled) system shows.
#
# Why this exists:
# When immutability is enabled with enforce.onReboot=true, the system runs
# /etc/nixos, /home/<user>, /var/log, /var/lib/sbctl, etc. as bind mounts
# from individual @persist/dirs/<key> subvolumes that stack on top of
# freshly-reset @root and @home. The underlying @root/etc/nixos and
# @home/<user> diverge from what the operator sees at the live paths.
#
# If immutability is then disabled (e.g. recovering from the P2.1 lockout
# bug while the structural fix is being rolled out), the next boot mounts
# @root and @home directly with no binds — exposing the divergent
# underlying content. Recent commits to /etc/nixos, KDE wallet state,
# Steam libraries, etc. that lived only in @persist disappear from view.
#
# This script flushes @persist subvolume content back into @root/@home so
# the divergence is resolved BEFORE the reboot into the immutability-off
# generation.
#
# Usage:
#   migrate-persist-to-root.sh --dry-run   # print plan, change nothing
#   migrate-persist-to-root.sh --apply     # do the migration
#
# Required:
#   - Run as root.
#   - Immutability service running on the current host (script reads
#     /etc/immutability/spec.tsv to discover persist paths).
#   - The block device backing the btrfs root present in /proc/mounts.

set -euo pipefail

SCRIPT_NAME=$(basename "$0")
DRY_RUN=true
SPEC_FILE=${IMMUTABILITY_SPEC:-/etc/immutability/spec.tsv}
SNAPSHOT_TAG="migrate-$$-$(date +%s)"
WORK_ROOT=${WORK_ROOT:-/run/migrate-$$}

die() {
    echo "${SCRIPT_NAME}: ERROR: $*" >&2
    exit 1
}

note() {
    echo "${SCRIPT_NAME}: $*" >&2
}

run() {
    if $DRY_RUN; then
        echo "DRY  $*"
    else
        echo "RUN  $*"
        "$@"
    fi
}

parse_args() {
    case "${1:-}" in
        --dry-run) DRY_RUN=true ;;
        --apply)   DRY_RUN=false ;;
        ""|--help|-h)
            cat <<EOF
Usage: $SCRIPT_NAME --dry-run | --apply

  --dry-run   Print every command that would run; touch nothing.
  --apply     Execute the migration. Side-effects: btrfs snapshots, mounts,
              rsync, umount. Does NOT reboot or rebuild the system.

Environment:
  IMMUTABILITY_SPEC  override /etc/immutability/spec.tsv path
  WORK_ROOT          override /run/migrate-<pid> staging dir
EOF
            exit 0
            ;;
        *) die "unknown argument: $1; pass --dry-run or --apply" ;;
    esac
}

preflight() {
    [[ $EUID -eq 0 ]] || die "must run as root"
    [[ -r "$SPEC_FILE" ]] || die "no immutability spec at $SPEC_FILE; is immutability service installed?"
    command -v btrfs >/dev/null || die "btrfs binary not in PATH"
    command -v rsync >/dev/null || die "rsync binary not in PATH"
    # lsof is OPTIONAL — only used for the informational open-fd report
    # in warn_open_fds. Missing lsof drops to a notice and continues.

    # Discover the btrfs device backing /
    DEVICE=$(findmnt -no SOURCE / | sed 's/\[.*//')
    [[ -n "$DEVICE" ]] || die "could not detect device backing /"
    note "btrfs device: $DEVICE"

    # @persist subvol layout (matches engine/constants.btrc — dirs() = "dirs",
    # NOT ".immutability/dirs" which is for the file-kind meta tree)
    PERSIST_ROOT="@persist"
    DIRS_SUBPATH="dirs"
    note "persist root: $PERSIST_ROOT/$DIRS_SUBPATH"

    note "spec file: $SPEC_FILE"
    note "snapshot tag: $SNAPSHOT_TAG"
    note "staging dir: $WORK_ROOT"
}

# Read spec.tsv lines into arrays. Format per ImmutabilityConfig.readSpecs:
#   <volume>\t<mountPoint>\t<absPath>\t<kind>
# We only migrate kind=auto (directories the engine treats as bind mounts).
read_spec() {
    SPEC_VOLUMES=()
    SPEC_ABSPATHS=()
    SPEC_KEYS=()
    while IFS=$'\t' read -r volume mount abspath kind; do
        [[ -z "$volume" || "$volume" == \#* ]] && continue
        # Match ImmutabilityPaths.key(): replace every / with ! and trim
        # leading ! (so /etc/nixos -> etc!nixos)
        key=${abspath#/}
        key=${key//\//!}
        SPEC_VOLUMES+=("$volume")
        SPEC_ABSPATHS+=("$abspath")
        SPEC_KEYS+=("$key")
    done < "$SPEC_FILE"
    [[ ${#SPEC_ABSPATHS[@]} -gt 0 ]] || die "spec is empty; nothing to migrate"
    note "discovered ${#SPEC_ABSPATHS[@]} persist paths in spec"
}

# Check that the bind-mount actually points to a persist subvol (i.e. the
# immutability runtime is active for this path). Skip paths that aren't
# bind-mounted right now — they're already on the underlying volume.
filter_active_binds() {
    ACTIVE_INDICES=()
    for i in "${!SPEC_ABSPATHS[@]}"; do
        local p=${SPEC_ABSPATHS[$i]}
        if findmnt --mountpoint "$p" >/dev/null 2>&1; then
            ACTIVE_INDICES+=("$i")
        else
            note "  skip $p (not bind-mounted; already on underlying volume)"
        fi
    done
    [[ ${#ACTIVE_INDICES[@]} -gt 0 ]] || die "no active persist binds found; nothing to migrate"
    note "${#ACTIVE_INDICES[@]} of ${#SPEC_ABSPATHS[@]} persist paths are actively bind-mounted"
}

# Warn about open file descriptors that would EBUSY the eventual side-mount
# umount. Doesn't block; operator decides. Skips entirely if lsof is absent.
warn_open_fds() {
    if ! command -v lsof >/dev/null; then
        note "(skipping open-fd report; lsof not in PATH — install it on the host for diagnostics)"
        return
    fi
    note "--- open file descriptors under persist paths (informational) ---"
    local found=0
    for i in "${ACTIVE_INDICES[@]}"; do
        local p=${SPEC_ABSPATHS[$i]}
        local out
        out=$(lsof +D "$p" 2>/dev/null | tail -n +2 | awk '{print $1}' | sort -u || true)
        if [[ -n "$out" ]]; then
            echo "  $p:" >&2
            echo "$out" | sed 's/^/    /' >&2
            found=1
        fi
    done
    [[ $found -eq 0 ]] && note "(none)"
    note "--- end open fd report ---"
}

# Take a read-only btrfs snapshot of every active persist subvol under
# @snapshots/<SNAPSHOT_TAG>/<key>. rsync runs from the snapshot so the
# migration is consistent under concurrent writes from running daemons.
snapshot_persist() {
    note "snapshotting active persist subvolumes for consistency"
    # Mount btrfs root to access @snapshots and @persist subvols by name
    run mkdir -p "$WORK_ROOT/btrfs"
    run mount -t btrfs -o subvolid=5 "$DEVICE" "$WORK_ROOT/btrfs"
    run mkdir -p "$WORK_ROOT/btrfs/@snapshots/$SNAPSHOT_TAG"
    for i in "${ACTIVE_INDICES[@]}"; do
        local key=${SPEC_KEYS[$i]}
        local src="$WORK_ROOT/btrfs/$PERSIST_ROOT/$DIRS_SUBPATH/$key"
        local dst="$WORK_ROOT/btrfs/@snapshots/$SNAPSHOT_TAG/$key"
        run btrfs subvolume snapshot -r "$src" "$dst"
    done
}

# Mount @root and @home subvolumes side-by-side so rsync has a destination
# that isn't covered by the bind mounts.
side_mount_targets() {
    note "side-mounting @root and @home"
    for sub in @root @home; do
        run mkdir -p "$WORK_ROOT/$sub"
        run mount -t btrfs -o "subvol=$sub" "$DEVICE" "$WORK_ROOT/$sub"
    done
}

# Decide which side-mount root a persist path migrates into based on its
# absolute path prefix. /home/* -> @home (relative path stripped of /home/),
# everything else -> @root.
target_for() {
    local p=$1
    if [[ $p == /home/* ]]; then
        echo "$WORK_ROOT/@home${p#/home}"
    else
        echo "$WORK_ROOT/@root$p"
    fi
}

# Rsync each snapshot into the corresponding side-mounted target.
# -aHAX preserves attrs, hardlinks, ACLs, xattrs. --delete makes the target
# byte-identical to the snapshot.
rsync_into_root() {
    note "rsyncing each persist subvol into the side-mounted target"
    for i in "${ACTIVE_INDICES[@]}"; do
        local key=${SPEC_KEYS[$i]}
        local abs=${SPEC_ABSPATHS[$i]}
        local src="$WORK_ROOT/btrfs/@snapshots/$SNAPSHOT_TAG/$key/"
        local dst="$(target_for "$abs")/"
        run mkdir -p "$dst"
        run rsync -aHAX --delete "$src" "$dst"
    done
}

# Cross-check: for /etc/nixos specifically, the .git HEAD in @root must now
# match the .git HEAD visible at the live path. Loudly surface mismatches.
verify_nixos_git() {
    if findmnt --mountpoint /etc/nixos >/dev/null 2>&1; then
        local live mig
        live=$(git -C /etc/nixos rev-parse HEAD 2>/dev/null || echo NONE)
        if $DRY_RUN; then
            note "verify: would compare /etc/nixos git HEAD ($live) vs migrated @root/etc/nixos"
        else
            mig=$(git -C "$WORK_ROOT/@root/etc/nixos" rev-parse HEAD 2>/dev/null || echo NONE)
            if [[ "$live" != "$mig" ]]; then
                die "git HEAD mismatch: live=$live migrated=$mig; refuse to continue"
            fi
            note "verify: /etc/nixos git HEAD $live present in @root"
        fi
    fi
}

# Tear down side mounts and the staging btrfs mount. Snapshots stay until
# operator manually deletes (so they have a rollback if reboot goes wrong).
cleanup() {
    note "unmounting side mounts and staging btrfs (snapshots retained for rollback)"
    run umount "$WORK_ROOT/@home" || true
    run umount "$WORK_ROOT/@root" || true
    run umount "$WORK_ROOT/btrfs" || true
    run rmdir "$WORK_ROOT/@home" "$WORK_ROOT/@root" "$WORK_ROOT/btrfs" "$WORK_ROOT" || true
}

main() {
    parse_args "${1:-}"
    if $DRY_RUN; then
        note "DRY-RUN MODE: no filesystem changes; pass --apply to execute"
    else
        note "APPLY MODE: filesystem changes will be made"
    fi
    note "step 1/8: preflight";            preflight
    note "step 2/8: read_spec";             read_spec
    note "step 3/8: filter_active_binds";   filter_active_binds
    note "step 4/8: warn_open_fds";         warn_open_fds
    note "step 5/8: snapshot_persist";      snapshot_persist
    note "step 6/8: side_mount_targets";    side_mount_targets
    note "step 7/8: rsync_into_root";       rsync_into_root
    note "step 8/8: verify + cleanup";      verify_nixos_git; cleanup
    note "done."
    note ""
    note "Next steps (operator):"
    note "  1. Verify @root/etc/nixos content if you haven't already."
    note "  2. nixos-rebuild switch (or wait if you've already activated a"
    note "     gen with immutability disabled)."
    note "  3. nix-env --delete-generations old --profile /nix/var/nix/profiles/system"
    note "     (removes lockout-bearing generations from the bootloader menu)."
    note "  4. Reboot. The system boots from @root with the same content"
    note "     you saw at the live persist paths."
    note "  5. To roll back: btrfs subvolume delete /<btrfs-root>/@snapshots/$SNAPSHOT_TAG/*"
}

main "$@"
