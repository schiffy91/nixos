{ config, lib, host, ... }:
let
  encrypted = config.settings.disk.storage ? sdcard;
  user = config.settings.users.admin.username;
  library = config.settings.apps.gaming.semu.drive;
  # Where the card is: its own mount point once it is encrypted, Jovian's automount until then.
  mount = if encrypted then config.settings.disk.storage.sdcard.mountPoint else "/run/media/${user}/${host.sdCard.label}";
  device = "/dev/disk/by-label/${host.sdCard.label}${lib.optionalString encrypted "-crypt"}";
in {
  # The library is ~/Drive's folder like on every machine; the Drive client keeps it on the
  # card, in a folder of its own there, and it is unavailable (not empty) while the card is out.
  # The card is one volume of `settings.disk.storage`, which is what moves the library to the encrypted
  # card. It was prepared (`nixosctl storage prepare`, which formats) on 2026-11-14.
  settings.disk.storage.sdcard = {
    label = host.sdCard.label;
    mountPoint = "/sd";
    keyFile = "/var/lib/nixos/sd-card/key";  # where this card's key has been since it was prepared
    library = "Emulation";
    driveFolder = library;
    steamLibrary = true;
  };
  settings.apps.utils.drive = {
    push = true;
    placements.${library} = "${mount}/drive";
    placementDevices."${mount}/drive" = device;
  };
}
