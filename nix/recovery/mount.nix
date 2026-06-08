{ config, lib, pkgs }:
let
  allVolumes = config.settings.disk.subvolumes.volumes;
  resetVolumes = lib.filter (volume: volume.resetOnBoot) allVolumes;
  trimPath = path: if path == "/" then path else lib.removeSuffix "/" path;
  pathDepth = path: builtins.length (lib.splitString "/" (lib.removePrefix "/" path));
  sortPaths = paths: lib.sort (left: right:
    let
      leftDepth = pathDepth left;
      rightDepth = pathDepth right;
    in if leftDepth == rightDepth then builtins.stringLength left < builtins.stringLength right else leftDepth < rightDepth
  ) paths;
  mountVolumes = lib.sort (left: right:
    let
      leftPath = trimPath left.mountPoint;
      rightPath = trimPath right.mountPoint;
      leftDepth = pathDepth leftPath;
      rightDepth = pathDepth rightPath;
    in if leftDepth == rightDepth then builtins.stringLength leftPath < builtins.stringLength rightPath else leftDepth < rightDepth
  ) allVolumes;
  pathsForVolume = volume:
    let
      mountPoint = volume.mountPoint;
      otherMounts = lib.filter (other: other.mountPoint != mountPoint && other.mountPoint != "/") allVolumes;
    in builtins.filter (path:
      if mountPoint == "/" then
        !(builtins.any (other: path == other.mountPoint || lib.hasPrefix (other.mountPoint + "/") path) otherMounts)
      else path == mountPoint || lib.hasPrefix (mountPoint + "/") path
  ) (map trimPath config.settings.disk.immutability.persist.paths);
  persistKey = import ../lib/immutability-key.nix { inherit lib; };
  legacyPersistKey = import ../lib/immutability-legacy-key.nix { inherit lib; };
  rawPersistMounts = lib.concatMap (volume:
    map (path: {
      inherit path;
      key = persistKey path;
      legacyKey = legacyPersistKey path;
      subvolume = "${config.settings.disk.immutability.persist.subvolumeRoot}/dirs/${persistKey path}";
    }) (sortPaths (pathsForVolume volume))
  ) resetVolumes;
  coveredBy = selected: path: builtins.any (mount:
    if mount.path == "/" then path != "/" else lib.hasPrefix (mount.path + "/") path
  ) selected;
  selectedPersistMounts = lib.foldl' (selected: mount:
    if coveredBy selected mount.path then selected else selected ++ [ mount ]
  ) [] rawPersistMounts;
  legacyUnambiguous = mount: !(builtins.any (other:
    other.path != mount.path && (other.key == mount.legacyKey || other.legacyKey == mount.legacyKey)
  ) selectedPersistMounts);
  persistMounts = map (mount: {
    inherit (mount) path subvolume;
    legacySubvolume =
      if mount.key != mount.legacyKey && legacyUnambiguous mount
      then "${config.settings.disk.immutability.persist.subvolumeRoot}/dirs/${mount.legacyKey}"
      else "";
  }) selectedPersistMounts;
  mountVolumeCommands = lib.concatMapStringsSep "\n" (volume: ''
    mount_volume ${lib.escapeShellArgs [
      volume.mountPoint
      volume.name
      (lib.concatStringsSep "," volume.mountOptions)
    ]}
  '') mountVolumes;
  mountPersistCommands = lib.concatMapStringsSep "\n" (mount: ''
    mount_persist_abs ${lib.escapeShellArgs [ mount.path mount.subvolume mount.legacySubvolume ]}
  '') persistMounts;
  rootDevice =
    if config.settings.disk.encryption.enable
    then config.settings.disk.by.mapper.root
    else config.settings.disk.by.partlabel.root;
  openRoot = lib.optionalString config.settings.disk.encryption.enable ''
    if [ ! -e "$root_device" ]; then
      cryptsetup open "$root_part" "$root_name"
    fi
  '';
  rootUnlockVariables = lib.optionalString config.settings.disk.encryption.enable ''
    root_part=${lib.escapeShellArg config.settings.disk.by.partlabel.root}
    root_name=${lib.escapeShellArg config.settings.disk.label.root}
  '';
in
pkgs.writeShellApplication {
  name = "nixos-mount";
  runtimeInputs = with pkgs; [ btrfs-progs coreutils cryptsetup util-linux ];
  text = ''
    set -euo pipefail

    if [ "''${1:-}" = "--help" ]; then
      printf 'Usage: nixos-mount [target]\n\n'
      printf 'Unlocks and mounts the installed NixOS system at target, defaulting to /mnt.\n'
      exit 0
    fi

    root_device=${lib.escapeShellArg rootDevice}
    ${rootUnlockVariables}
    boot_part=${lib.escapeShellArg config.settings.disk.by.partlabel.boot}
    recovery_part=${lib.escapeShellArg config.settings.disk.by.partlabel.recovery}
    target="''${1:-/mnt}"
    top=/run/nixos-top

    case "$target" in
      /*) ;;
      *) printf 'target must be absolute: %s\n' "$target" >&2; exit 1 ;;
    esac

    ${openRoot}

    device_without_subvolume() {
      printf '%s\n' "''${1%%\[*}"
    }

    same_device() {
      local left
      local right

      left="$(device_without_subvolume "$1")"
      right="$(device_without_subvolume "$2")"

      [ -e "$left" ] && [ -e "$right" ] && [ "$(readlink -f "$left")" = "$(readlink -f "$right")" ]
    }

    fail_existing_mount() {
      local mount_target="$1"

      printf 'mountpoint already mounted differently: %s\n' "$mount_target" >&2
      findmnt -rn --mountpoint "$mount_target" >&2 || true
      exit 1
    }

    target_path() {
      if [ "$1" = "/" ]; then
        printf '%s\n' "$target"
      else
        printf '%s%s\n' "$target" "$1"
      fi
    }

    mount_btrfs() {
      local mount_target="$1"
      local subvolume="$2"
      local extra_options="''${3:-}"
      local options="subvol=$subvolume"

      if [ -n "$extra_options" ]; then
        options="$options,$extra_options"
      fi

      mkdir -p "$mount_target"
      if findmnt -rn --mountpoint "$mount_target" >/dev/null; then
        local fstype
        local source
        local current_options
        local subvolume_match=0

        fstype="$(findmnt -rn --mountpoint "$mount_target" -o FSTYPE)"
        source="$(findmnt -rn --mountpoint "$mount_target" -o SOURCE)"
        current_options="$(findmnt -rn --mountpoint "$mount_target" -o OPTIONS)"

        case "$source" in
          *"[$subvolume]") subvolume_match=1 ;;
        esac
        case ",$current_options," in
          *",subvol=$subvolume,"*|*",subvol=/$subvolume,"*) subvolume_match=1 ;;
        esac

        if [ "$fstype" = btrfs ] && same_device "$source" "$root_device" && [ "$subvolume_match" = 1 ]; then
          return 0
        fi
        fail_existing_mount "$mount_target"
      fi
      mount -t btrfs -o "$options" "$root_device" "$mount_target"
    }

    mount_volume() {
      mount_btrfs "$(target_path "$1")" "$2" "''${3:-}"
    }

    mount_device() {
      local mount_target="$1"
      local device="$2"
      local required="$3"

      mkdir -p "$mount_target"
      if current="$(findmnt -rn --mountpoint "$mount_target" -o SOURCE)"; then
        if same_device "$current" "$device"; then
          return 0
        fi
        fail_existing_mount "$mount_target"
      fi
      if [ ! -e "$device" ]; then
        if [ "$required" = 1 ]; then
          printf 'missing device: %s\n' "$device" >&2
          exit 1
        fi
        return 0
      fi
      mount "$device" "$mount_target"
    }

    mount_top() {
      mkdir -p "$top"
      if findmnt -rn --mountpoint "$top" >/dev/null; then
        local fstype
        local source
        local current_options

        fstype="$(findmnt -rn --mountpoint "$top" -o FSTYPE)"
        source="$(findmnt -rn --mountpoint "$top" -o SOURCE)"
        current_options="$(findmnt -rn --mountpoint "$top" -o OPTIONS)"
        if [ "$fstype" = btrfs ] && same_device "$source" "$root_device"; then
          case ",$current_options," in
            *",subvolid=5,"*) return 0 ;;
          esac
        fi
        fail_existing_mount "$top"
      fi
      mount -t btrfs -o ro,subvolid=5 "$root_device" "$top"
    }

    selected_targets=()

    covered_by_selected() {
      local mount_target="$1"
      local selected

      for selected in "''${selected_targets[@]}"; do
        case "$mount_target" in
          "$selected"/*) return 0 ;;
        esac
      done
      return 1
    }

    mount_persist_abs() {
      local path="$1"
      local subvolume="$2"
      local legacy_subvolume="$3"
      local mount_target
      local actual_subvolume="$subvolume"

      mount_target="$(target_path "$path")"
      if covered_by_selected "$mount_target"; then
        return 0
      fi
      if [ -n "$legacy_subvolume" ] && ! btrfs subvolume show "$top/$actual_subvolume" >/dev/null 2>&1; then
        actual_subvolume="$legacy_subvolume"
      fi
      if ! btrfs subvolume show "$top/$actual_subvolume" >/dev/null 2>&1; then
        return 0
      fi
      mount_btrfs "$mount_target" "$actual_subvolume" "compress=zstd,noatime"
      selected_targets+=("$mount_target")
    }

    ${mountVolumeCommands}

    mkdir -p "$(target_path /tmp)"
    chmod 1777 "$(target_path /tmp)"

    mount_top
    ${mountPersistCommands}

    mount_device "$(target_path /boot)" "$boot_part" 1
    mount_device "$(target_path /recovery)" "$recovery_part" 0
    findmnt -R "$target"
  '';
}
