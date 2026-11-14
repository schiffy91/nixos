{ config, lib, host, ... }:
let
  card = config.settings.disk.sdCard;
  user = config.settings.users.admin.username;
  library = config.settings.apps.gaming.semu.drive;
  # Where the card is: its own mount point once it is encrypted, Jovian's automount until then.
  mount = if card.enable then card.mountPoint else "/run/media/${user}/${host.sdCard.label}";
  device = "/dev/disk/by-label/${host.sdCard.label}${lib.optionalString card.enable "-crypt"}";
in {
  # The library is ~/Drive's folder like on every machine; the Drive client keeps it on the
  # card, in a folder of its own there, and it is unavailable (not empty) while the card is out.
  # `settings.disk.sdCard.enable` moves it to the encrypted card: turn it on only together with
  # preparing the card (`nixosctl sd-card prepare`), which formats it.
  settings.disk.sdCard.label = host.sdCard.label;
  settings.apps.utils.drive = {
    push = true;
    placements.${library} = "${mount}/drive";
    placementDevices."${mount}/drive" = device;
  };
}
