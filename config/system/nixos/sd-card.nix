{ config, lib, pkgs, utils, ... }:
let
  card = config.settings.disk.sdCard;
  name = "sdcard";
  container = "/dev/disk/by-label/${card.label}-crypt";
in {
  config = lib.mkIf card.enable {
    environment.systemPackages = [ pkgs.cryptsetup ];
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
  };
}
