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
      script = "exec ${../../bin/immutability.sh}";
    };
  };
}