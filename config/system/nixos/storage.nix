{ config, lib, pkgs, utils, ... }:
let
  volumes = config.settings.disk.storage;
  each = make: lib.mapAttrsToList make volumes;
  container = volume: "/dev/disk/by-label/${volume.label}-crypt";
  mount = volume: "${utils.escapeSystemdPath volume.mountPoint}.mount";
  user = config.users.users.${config.settings.users.admin.username};
  withSteam = lib.optionalAttrs config.programs.steam.enable (lib.filterAttrs (_: volume: volume.steamLibrary) volumes);
in {
  config = lib.mkIf (volumes != { }) {
    environment.systemPackages = [ pkgs.cryptsetup pkgs.e2fsprogs ];  # preparing a volume makes its filesystem; a machine with no ext4 of its own has no mkfs.ext4
    # What `nixosctl storage` needs to know, a line for each volume: its name, the labels and paths above, the
    # one folder a device may hold and still be formatted with where that folder is on the Drive, and its owner.
    environment.etc."nixos-storage".text = lib.concatStrings (each (name: volume:
      lib.concatStringsSep "\t" [ name volume.label volume.mountPoint volume.keyFile volume.library volume.driveFolder user.name ] + "\n"));
    # Opened with the key file and never at boot's request: a volume that is not there costs
    # boot nothing. The rule below asks for it whenever the device appears, at boot or later.
    environment.etc.crypttab.text = lib.concatStrings (each (name: volume: "${name} ${container volume} ${volume.keyFile} luks,noauto,nofail\n"));
    services.udev.extraRules = lib.concatStrings (each (name: volume: ''
      ACTION!="remove", SUBSYSTEM=="block", ENV{ID_FS_TYPE}=="crypto_LUKS", ENV{ID_FS_LABEL}=="${volume.label}-crypt", TAG+="systemd", ENV{SYSTEMD_WANTS}+="systemd-cryptsetup@${name}.service"
    ''));
    # Mount units rather than fstab lines, for the lazy unmount: a pulled device must come
    # off its mount point at once even while something still holds a file on it.
    systemd.mounts = each (name: volume: {
      what = "/dev/mapper/${name}";
      where = volume.mountPoint;
      type = "ext4";
      options = "noatime";
      wantedBy = [ "dev-mapper-${name}.device" ];
      # Bound to the device itself as well as the opened container: the container outlives a
      # pulled device for as long as it is mounted, the device does not. Not bound to the
      # unit that opens it, so a volume opened by hand with the password mounts too.
      bindsTo = [ "dev-mapper-${name}.device" "${utils.escapeSystemdPath (container volume)}.device" ];
      after = [ "dev-mapper-${name}.device" "${utils.escapeSystemdPath (container volume)}.device" ];
      mountConfig.LazyUnmount = true;
    });
    systemd.services = lib.mkMerge [
      # Close the container once its last user lets go, so the same device can be opened again.
      (lib.mapAttrs' (name: _: lib.nameValuePair "systemd-cryptsetup@${name}" {
        overrideStrategy = "asDropin";
        serviceConfig.ExecStopPost = "-${pkgs.cryptsetup}/bin/cryptsetup close --deferred ${name}";
      }) volumes)
      # The boot password also opens a volume (so any machine can, by hand). Let the key files go first.
      { nixos-opportunistic-unlock.after = lib.mkIf config.settings.disk.encryption.opportunisticUnlock.enable (each (name: _: "systemd-cryptsetup@${name}.service")); }
      # Steam keeps its library in a folder of its own on the volume, and a running Steam is told when the
      # volume arrives, as SteamOS tells it about a card it mounts itself. Steam then lists the volume under
      # Storage and remembers it; it is not told when the volume goes, so a restart does not make it forget.
      (lib.mapAttrs' (name: volume: lib.nameValuePair "storage-steam-library-${name}" {
        description = "Steam's library on ${volume.mountPoint}";
        wantedBy = [ (mount volume) ];
        bindsTo = [ (mount volume) ];
        after = [ (mount volume) ];
        environment.XDG_RUNTIME_DIR = "/run/user/${toString user.uid}";
        serviceConfig = {
          Type = "oneshot";
          RemainAfterExit = true;
          User = user.name;
          TimeoutSec = 60;
          ExecStartPre = "+${pkgs.coreutils}/bin/install -d -o ${user.name} -g ${user.group} -m 0755 ${volume.mountPoint}/steam";  # as root: the volume's top folder is root's
          # A Steam that is not running reads its own list when it starts. Each % is doubled: systemd reads one as a specifier.
          ExecStart = "-${config.programs.steam.package}/bin/steam -ifrunning steam://addlibraryfolder/${lib.replaceStrings [ "%" ] [ "%%" ] (lib.strings.escapeURL "${volume.mountPoint}/steam")}";
        };
      }) withSteam)
    ];
    systemd.tmpfiles.rules = each (_: volume: "d ${volume.mountPoint} 0755 root root -");
  };
}
