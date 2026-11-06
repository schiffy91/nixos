{ system ? builtins.currentSystem, rotate ? false }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree {
    inherit (locked) type owner repo rev narHash;
  }) { inherit system; };
  input = pkgs.runCommandCC "plymouth-test-input" { } ''
    mkdir -p $out/bin
    $CC -Wall -Wextra -Werror ${./input.c} -o $out/bin/test-input
  '';
in pkgs.testers.runNixOSTest {
  name = "plymouth-touch-unlock" + pkgs.lib.optionalString rotate "-rotated";
  extraPythonPackages = packages: [ packages.pillow ];
  nodes.machine = { lib, pkgs, ... }: {
    imports = [ ../../nix/settings.nix ../../nix/system/nixos/boot/touch-keyboard.nix ];
    settings.boot.touchKeyboard = true;
    boot.plymouth = { enable = true; theme = "bgrt"; };
    virtualisation = {
      emptyDiskImages = [ 512 ];
      useBootLoader = true;
      mountHostNixStore = true;
      useEFIBoot = true;
      qemu.options = if rotate then [ "-vga none" "-device virtio-vga,xres=800,yres=1280" ] else [ "-vga virtio" ];
    };
    boot.loader.systemd-boot.enable = true;
    boot.initrd.kernelModules = [ "virtio_gpu" "uinput" ];
    boot.kernelParams = [ "console=tty0" "plymouth.ignore-serial-consoles" ]
      ++ lib.optionals rotate [ "fbcon=rotate:1" "video=Virtual-1:800x1280,panel_orientation=right_side_up" ];
    environment.systemPackages = [ pkgs.cryptsetup ];
    # The test driver adds keyboards unrelated to the user's attached devices.
    # The test's separate USB uinput keyboard is counted normally.
    boot.initrd.services.udev.rules = ''
      SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="AT Translated Set 2 keyboard", ENV{PLYMOUTH_IGNORE_KEYBOARD}="1"
      SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="QEMU Virtio Keyboard", ENV{PLYMOUTH_IGNORE_KEYBOARD}="1"
    '';
    specialisation.encrypted.configuration = {
      testing.initrdBackdoor = true;
      boot.initrd.systemd.storePaths = [ input "${pkgs.coreutils}/bin/true" ];
      # Prevent udev from loading the GPU before the deliberately delayed
      # modules-load service. The explicit module load still works normally.
      boot.initrd.services.udev.packages = [ (pkgs.writeTextDir "etc/udev/rules.d/01-defer-test-gpu.rules" ''
        SUBSYSTEM=="virtio", ENV{MODALIAS}=="virtio:d00000010*", RUN:="${pkgs.coreutils}/bin/true"
      '') ];
      boot.initrd.luks.devices = lib.mkVMOverride {
        cryptroot.device = "/dev/vdb";
      };
      virtualisation.rootDevice = "/dev/mapper/cryptroot";
      boot.initrd.systemd.services = {
        systemd-modules-load.preStart = ''
          ${pkgs.coreutils}/bin/sleep 10
          test ! -d /sys/module/virtio_gpu
          ${pkgs.coreutils}/bin/ls /run/systemd/ask-password/ask.* > /run/request-before-drivers
        '';
        test-input = {
          wantedBy = [ "sysinit.target" ];
          after = [ "systemd-modules-load.service" ];
          before = [ "plymouth-start.service" ];
          unitConfig.DefaultDependencies = false;
          serviceConfig.Type = "exec";
          serviceConfig.ExecStart = "${input}/bin/test-input " + (if rotate then "keyboard" else "touchscreen");
        };
      };
    };
  };
  testScript = { nodes, ... }: builtins.replaceStrings
    [ "@ROTATED@" "@SYSTEM@" ]
    [ (if rotate then "True" else "False") "${nodes.machine.specialisation.encrypted.configuration.system.build.toplevel}" ]
    (builtins.readFile ./test.py);
}
