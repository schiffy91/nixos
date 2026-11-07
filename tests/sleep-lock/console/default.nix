{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree {
    inherit (locked) type owner repo rev narHash;
  }) { inherit system; };
  fixtures = pkgs.runCommandCC "sleep-console-fixtures" { } ''
    mkdir -p $out/bin
    $CC -Wall -Wextra -Werror ${./vt.c} -o $out/bin/test-vt
    $CC -Wall -Wextra -Werror ${../../plymouth-keyboard/input.c} -o $out/bin/test-input
  '';
in pkgs.testers.runNixOSTest {
  name = "sleep-lock-console";
  enableOCR = true;
  extraPythonPackages = packages: [ packages.pillow ];
  nodes.machine = { config, pkgs, ... }: {
    imports = [ ../../../nix/settings.nix ../../../nix/system/nixos/boot/touch-keyboard.nix ];
    settings.boot.touchKeyboard = true;
    boot.plymouth.enable = true;
    boot.plymouth.theme = "bgrt";
    boot.kernelModules = [ "virtio_gpu" "uinput" ];
    boot.kernelParams = [ "console=tty0" "plymouth.ignore-serial-consoles" ];
    virtualisation.qemu.options = [ "-vga virtio" ];
    virtualisation.memorySize = 4096;
    virtualisation.cores = 2;
    users.users.observer = {
      isNormalUser = true;
      uid = 1000;
    };
    services.xserver.enable = true;
    services.desktopManager.plasma6.enable = true;
    services.displayManager.sddm = {
      enable = true;
      wayland.enable = true;
    };
    services.displayManager.autoLogin = {
      enable = true;
      user = "observer";
    };
    services.displayManager.defaultSession = "plasma";
    programs.kde-pim.enable = false;
    environment.systemPackages = [ fixtures ];
    services.udev.extraRules = ''
      SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="AT Translated Set 2 keyboard", ENV{PLYMOUTH_IGNORE_KEYBOARD}="1"
      SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="QEMU Virtio Keyboard", ENV{PLYMOUTH_IGNORE_KEYBOARD}="1"
    '';
    systemd.services.test-input = {
      wantedBy = [ "multi-user.target" ];
      serviceConfig.ExecStart = "${fixtures}/bin/test-input touchscreen";
    };
    # This is a rendering/boundary experiment, not a deployed screen locker.
    # A real locker must additionally authenticate and order sleep safely.
    systemd.services.test-lock-ui = {
      environment = {
        LANG = "C.UTF-8";
        XKB_CONFIG_ROOT = "${pkgs.xkeyboard-config}/share/X11/xkb";
        XLOCALEDIR = "${pkgs.libx11}/share/X11/locale";
      };
      serviceConfig = {
        ExecStart = "${config.boot.plymouth.package}/bin/plymouthd --no-daemon --no-boot-log --tty=/dev/tty63 --mode=boot-up --pid-file=/run/test-lock-ui.pid";
        ExecStop = "${config.boot.plymouth.package}/bin/plymouth quit";
        Restart = "always";
        RestartSec = 1;
      };
    };
  };
  testScript = builtins.readFile ./test.py;
}
