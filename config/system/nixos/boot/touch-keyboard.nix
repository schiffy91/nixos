{ config, lib, pkgs, ... }:
let
  # Plymouth consumes XKB settings from vconsole.conf; KEYMAP alone only
  # configures the legacy console input path.
  vconsole = withFont: pkgs.writeText "plymouth-vconsole.conf" ''
    KEYMAP=${config.console.keyMap}
    ${lib.optionalString (withFont && config.console.font != null) "FONT=${config.console.font}"}
    XKBLAYOUT=${config.services.xserver.xkb.layout}
    XKBMODEL=${config.services.xserver.xkb.model}
    XKBVARIANT=${config.services.xserver.xkb.variant}
    XKBOPTIONS=${config.services.xserver.xkb.options}
  '';
in {

  boot.plymouth.package = pkgs.callPackage ./plymouth/package.nix {
    systemd = config.boot.initrd.systemd.package;
  };
  boot.initrd.availableKernelModules = lib.optionals config.settings.boot.touchKeyboard [
    "hid-multitouch" "hid-generic" "usbhid" "i2c-designware-core"
    "i2c-designware-platform" "i2c-hid-acpi" "usbtouchscreen" "evdev"
  ];
  environment.etc."vconsole.conf".source = lib.mkIf config.settings.boot.touchKeyboard
    (lib.mkForce (vconsole true));
  boot.initrd.systemd.contents."/etc/vconsole.conf".source = lib.mkIf config.settings.boot.touchKeyboard
    (lib.mkForce (vconsole config.console.earlySetup));
  boot.initrd.systemd.storePaths = lib.optionals config.settings.boot.touchKeyboard [
    pkgs.xkeyboard-config
    "${pkgs.libx11}/share/X11/locale"
    "${pkgs.glibc}/lib/locale/C.utf8"
  ];
  boot.initrd.systemd.services.plymouth-start = lib.mkIf config.boot.plymouth.enable {
    environment = lib.mkIf config.settings.boot.touchKeyboard {
      LANG = "C.UTF-8";
      LOCPATH = "${pkgs.glibc}/lib/locale";
      XKB_CONFIG_ROOT = "${pkgs.xkeyboard-config}/share/X11/xkb";
      XLOCALEDIR = "${pkgs.libx11}/share/X11/locale";
    };
    wants = [ "systemd-modules-load.service" ];
    after = [ "systemd-modules-load.service" ];
  };
}
