{ config, immutabilityPersistKey, inputs, lib, pkgs }:
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
  persistKey = immutabilityPersistKey;
  rawPersistMounts = lib.concatMap (volume:
    map (path: {
      inherit path;
      subvolume = "${config.settings.disk.immutability.persist.subvolumeRoot}/dirs/${persistKey path}";
    }) (sortPaths (pathsForVolume volume))
  ) resetVolumes;
  coveredBy = selected: path: builtins.any (mount:
    if mount.path == "/" then path != "/" else lib.hasPrefix (mount.path + "/") path
  ) selected;
  selectedPersistMounts = lib.foldl' (selected: mount:
    if coveredBy selected mount.path then selected else selected ++ [ mount ]
  ) [] rawPersistMounts;
  persistMounts = selectedPersistMounts;
  plan = pkgs.writeText "nixos-mount.json" (builtins.toJSON {
    encrypted = config.settings.disk.encryption.enable;
    rootDevice = if config.settings.disk.encryption.enable then config.settings.disk.by.mapper.root else config.settings.disk.by.partlabel.root;
    rootPartition = config.settings.disk.by.partlabel.root;
    rootName = config.settings.disk.label.root;
    bootPartition = config.settings.disk.by.partlabel.boot;
    recoveryPartition = config.settings.disk.by.partlabel.recovery;
    volumes = map (volume: {
      inherit (volume) mountPoint name;
      options = lib.concatStringsSep "," volume.mountOptions;
    }) mountVolumes;
    persist = persistMounts;
  });
in
(import ../../../lib/btrc.nix { inherit pkgs inputs; }) {
  name = "nixos-mount";
  src = lib.fileset.toSource {
    root = ./.;
    fileset = lib.fileset.unions [ ./Main.btrc ./Mount.btrc ];
  };
  programs.nixos-mount = "Main.btrc";
  runtimeInputs = with pkgs; [ btrfs-progs coreutils cryptsetup util-linux ];
  wrapperArgs = [ "--add-flags" (lib.escapeShellArg plan) ];
}
