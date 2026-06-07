{ lib, mkDiskoSystem }: { pkgs, hostFile, target, name }:
let
  diskSystem = mkDiskoSystem hostFile target;
  settings = diskSystem.config.settings.disk;
  disk = builtins.getAttr settings.label.main diskSystem.config.disko.devices.disk;
  partitions = disk.content.partitions;
  boot = builtins.getAttr settings.label.boot partitions;
  root = builtins.getAttr settings.label.root partitions;
  rootContent = root.content;
  hasRecovery = builtins.hasAttr settings.label.recovery partitions;
  recovery =
    if hasRecovery
    then builtins.getAttr settings.label.recovery partitions
    else null;
  btrfs =
    if rootContent.type == "luks"
    then rootContent.content
    else rootContent;
  subvolumeNames = builtins.attrNames btrfs.subvolumes;
  expect = label: actual: expected:
    if actual == expected
    then actual
    else throw "${name}: expected ${label} to be ${builtins.toJSON expected}, got ${builtins.toJSON actual}";
  expectContains = label: value: values:
    if builtins.elem value values
    then values
    else throw "${name}: expected ${label} to contain ${builtins.toJSON value}";
  subvolumeContract =
    map (volume: expectContains "root btrfs subvolumes" volume.name subvolumeNames)
      settings.subvolumes.volumes;
  recoveryContract = lib.optionals hasRecovery [
    (expect "recovery.type" recovery.type "EF00")
    (expect "recovery.label" recovery.label settings.partlabel.recovery)
    (expect "recovery.content.format" recovery.content.format "vfat")
    (expect "recovery.content.mountpoint" recovery.content.mountpoint settings.recovery.mountPoint)
    (expectContains "recovery.content.extraArgs" "-n" recovery.content.extraArgs)
    (expectContains "recovery.content.extraArgs" settings.recovery.filesystemLabel recovery.content.extraArgs)
    (expectContains "recovery.content.mountOptions" "nofail" recovery.content.mountOptions)
  ];
  contract = [
    (expect "disk.type" disk.type "disk")
    (expect "disk.content.type" disk.content.type "gpt")
    (expect "boot.type" boot.type "EF00")
    (expect "boot.label" boot.label settings.partlabel.boot)
    (expect "boot.content.format" boot.content.format "vfat")
    (expect "boot.content.mountpoint" boot.content.mountpoint settings.boot.efiSysMountPoint)
    (expect "root.size" root.size "100%")
    (expect "root.label" root.label settings.partlabel.root)
    (expect "root.content.type" rootContent.type (if settings.encryption.enable then "luks" else "btrfs"))
    (expect "root filesystem type" btrfs.type "btrfs")
    (expect "recovery partition enabled" hasRecovery settings.recovery.enable)
  ] ++ lib.optionals settings.encryption.enable [
    (expect "root LUKS name" rootContent.name settings.label.root)
  ] ++ subvolumeContract ++ recoveryContract;
  checked = builtins.deepSeq contract name;
in {
  name = "disko-${name}";
  value = pkgs.runCommand "check-${name}" { } ''
    printf '%s\n' ${lib.escapeShellArg checked} > "$out"
  '';
}
