{ config, lib, pkgs, utils, ... }:
let
  card = config.settings.disk.sdCard;
  name = "sdcard";
  container = "/dev/disk/by-label/${card.label}-crypt";
  mount = "${utils.escapeSystemdPath card.mountPoint}.mount";
  user = config.users.users.${config.settings.users.admin.username};
  addLibrary = "-${config.programs.steam.package}/bin/steam -ifrunning steam://addlibraryfolder/${lib.strings.escapeURL "${card.mountPoint}/steam"}";
in {
  config = lib.mkIf card.enable {
    environment.systemPackages = [ pkgs.cryptsetup pkgs.e2fsprogs ];  # preparing a card makes its filesystem; a machine with no ext4 of its own has no mkfs.ext4
    # What `nixosctl sd-card` needs to know: the labels and paths above, and the one folder a
    # card may hold and still be formatted, with where that folder is on the Drive.
    environment.etc."nixos-sd-card".text = lib.concatStringsSep "\n" [ card.label card.mountPoint card.keyFile card.library card.driveFolder config.settings.users.admin.username ] + "\n";
    # Opened with the key file and never at boot's request: a card that is not there costs
    # boot nothing. The rule below asks for it whenever the card appears, at boot or later.
    environment.etc.crypttab.text = "${name} ${container} ${card.keyFile} luks,noauto,nofail\n";
    services.udev.extraRules = ''
      ACTION!="remove", SUBSYSTEM=="block", ENV{ID_FS_TYPE}=="crypto_LUKS", ENV{ID_FS_LABEL}=="${card.label}-crypt", TAG+="systemd", ENV{SYSTEMD_WANTS}+="systemd-cryptsetup@${name}.service"
    '';
    # A mount unit rather than an fstab line, for the lazy unmount: a pulled card must come
    # off its mount point at once even while something still holds a file on it.
    systemd.mounts = [ {
      what = "/dev/mapper/${name}";
      where = card.mountPoint;
      type = "ext4";
      options = "noatime";
      wantedBy = [ "dev-mapper-${name}.device" ];
      # Bound to the card itself as well as the opened container: the container outlives a
      # pulled card for as long as it is mounted, the card's own device does not. Not bound
      # to the unit that opens it, so a card opened by hand with the password mounts too.
      bindsTo = [ "dev-mapper-${name}.device" "${utils.escapeSystemdPath container}.device" ];
      after = [ "dev-mapper-${name}.device" "${utils.escapeSystemdPath container}.device" ];
      mountConfig.LazyUnmount = true;
    } ];
    # Close the container once its last user lets go, so the same card can be opened again.
    systemd.services."systemd-cryptsetup@${name}" = {
      overrideStrategy = "asDropin";
      serviceConfig.ExecStopPost = "-${pkgs.cryptsetup}/bin/cryptsetup close --deferred ${name}";
    };
    # The boot password also opens the card (so any machine can, by hand). Let the key file go first.
    systemd.services.nixos-opportunistic-unlock.after = lib.mkIf config.settings.disk.encryption.opportunisticUnlock.enable [ "systemd-cryptsetup@${name}.service" ];
    systemd.tmpfiles.rules = [ "d ${card.mountPoint} 0755 root root -" ];
    # Where there is a Steam it keeps a library on the card, in a folder of its own, and a running Steam is
    # told when the card arrives, as SteamOS tells it about a card it mounts itself. Steam then lists the card
    # under Storage and remembers it; it is not told when the card goes, so a restart does not make it forget.
    systemd.services.sd-card-steam-library = lib.mkIf config.programs.steam.enable {
      description = "Steam's library on the card";
      wantedBy = [ mount ];
      bindsTo = [ mount ];
      after = [ mount ];
      environment.XDG_RUNTIME_DIR = "/run/user/${toString user.uid}";
      serviceConfig = {
        Type = "oneshot";
        RemainAfterExit = true;
        User = user.name;
        TimeoutSec = 60;
        ExecStartPre = "+${pkgs.coreutils}/bin/install -d -o ${user.name} -g ${user.group} -m 0755 ${card.mountPoint}/steam";  # as root: the card's top folder is root's
        ExecStart = addLibrary;  # a Steam that is not running reads the folder from its own list when it starts
      };
    };
  };
}
