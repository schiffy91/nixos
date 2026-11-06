{ config, lib, pkgs, ... }:
{
  boot.plymouth.package = pkgs.callPackage ./plymouth/package.nix {
    systemd = config.boot.initrd.systemd.package;
  };
  boot.initrd.availableKernelModules = lib.optionals config.settings.boot.touchKeyboard [
    "hid-multitouch" "hid-generic" "usbhid" "i2c-designware-core"
    "i2c-designware-platform" "i2c-hid-acpi" "usbtouchscreen" "evdev"
  ];
  boot.initrd.systemd.services.plymouth-start = lib.mkIf config.boot.plymouth.enable {
    wants = [ "systemd-modules-load.service" ];
    after = [ "systemd-modules-load.service" ];
  };
}
