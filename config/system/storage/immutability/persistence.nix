{ config, lib, ... }: {
  settings.disk.immutability.persist.paths = [
    "/etc/machine-id"
    "/etc/nixos"
    "/usr/bin/env"
    "/var/lib/nixos"
    "/var/lib/systemd/coredump"
    "/var/log"
  ] ++ lib.optional config.hardware.bluetooth.enable "/var/lib/bluetooth";
}
