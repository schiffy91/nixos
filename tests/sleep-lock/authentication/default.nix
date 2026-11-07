{ system ? builtins.currentSystem, lockSession ? true }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree {
    inherit (locked) type owner repo rev narHash;
  }) { inherit system; };
  fixtures = pkgs.runCommandCC "sleep-console-fixtures" { } ''
    mkdir -p $out/bin
    $CC -Wall -Wextra -Werror ${../../plymouth-keyboard/input.c} -o $out/bin/test-input
  '';
in pkgs.testers.runNixOSTest {
  node.pkgsReadOnly = false;
  name = if lockSession then "sleep-lock-authentication" else "sleep-suspend-control";
  enableOCR = true;
  extraPythonPackages = packages: [ packages.pillow ];
  nodes.machine = { config, pkgs, lib, ... }: {
    imports = [ ../../../nix/settings.nix ../../../nix/system/nixos/boot/touch-keyboard.nix ../../../nix/system/nixos/sleep.nix ];
    settings.boot.touchKeyboard = true;
    settings.sleep.lockSession = lockSession;
    settings.users.admin.username = "observer";
    boot.plymouth.enable = true;
    boot.plymouth.theme = "bgrt";
    boot.kernelPackages = pkgs.linuxPackages_latest;
    boot.kernelModules = [ "virtio_gpu" "uinput" ];
    boot.kernelParams = [ "console=tty0" "plymouth.ignore-serial-consoles" "no_console_suspend" ];
    # Linux 7.3 provides virtio-gpu freeze/restore callbacks; 6.18 loses
    # scanout across sleep even in the unlocked desktop control.
    virtualisation.qemu.options = [ "-vga virtio" "-nodefaults" "-global virtio-vga.x-pcie-pm-no-soft-reset=on" ];
    virtualisation.memorySize = 4096;
    virtualisation.cores = 2;
    # virtio-fs refuses suspend. Keep the test closure on an ephemeral EROFS
    # disk and use the serial test backdoor instead of shared filesystems.
    virtualisation.useNixStoreImage = true;
    virtualisation.writableStore = true;
    virtualisation.sharedDirectories = lib.mkForce { };
    environment.etc."xdg/kscreenlockerrc".text = ''
      [Daemon]
      LockOnResume=false
    '';
    users.users.observer = {
      isNormalUser = true;
      uid = 1000;
      password = "a";
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
    systemd.sleep.settings.Sleep.MemorySleepMode = "s2idle";
    services.logind.settings.Login.HandlePowerKey = "ignore";
    # Exercise real sleep unit pre/post ordering with an injected backend.
    # The final case delegates to the real suspend helper and wakes from a simulated ACPI power button.
    systemd.services.systemd-suspend.serviceConfig.ExecStart = lib.mkForce [ ""
      "${pkgs.writeShellScript "sleep-backend" ''
        ${lib.optionalString lockSession ''test "$(cat /sys/class/tty/tty0/active)" = tty63 || exit 90''}
        touch /run/sleep-backend-entered
        if test -e /run/real-suspend; then
          exec ${config.systemd.package}/lib/systemd/systemd-sleep suspend
        fi
        while test -e /run/hold-sleep-backend; do sleep .1; done
        if test -e /run/fail-sleep-backend; then exit 1; fi
      ''}"
    ];
    environment.etc."sleep-lock-controller" = lib.mkIf lockSession {
      text = config.systemd.services.nixos-sleep-lock.serviceConfig.ExecStart;
    };

  };
  testScript = builtins.readFile (if lockSession then ./test.py else ../suspend-control/test.py);
}
