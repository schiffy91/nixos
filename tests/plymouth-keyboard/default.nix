{ system ? builtins.currentSystem, rotate ? false, layout ? "us", light ? false, multi ? false, scale ? 1 }:
assert builtins.elem layout [ "us" "de" "fr" ];
assert builtins.elem scale [ 1 2 ];
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
  name = "plymouth-touch-unlock-${layout}" + pkgs.lib.optionalString rotate "-rotated";
  extraPythonPackages = packages: [ packages.pillow ];
  nodes.machine = { config, lib, pkgs, ... }: {
    imports = [ ../../config/settings.nix ../../config/system/nixos/boot/touch-keyboard.nix ];
    settings.boot.touchKeyboard = true;
    services.xserver.xkb.layout = "${layout},${if layout == "us" then "de" else "us"}";
    boot.plymouth = {
      enable = true;
      theme = "osk-test";
      themePackages = [ (pkgs.runCommand "osk-test-theme" { } ''
        mkdir -p $out/share/plymouth/themes/osk-test
        cat ${config.boot.plymouth.package}/share/plymouth/themes/bgrt/bgrt.plymouth > $out/share/plymouth/themes/osk-test/osk-test.plymouth
        cat >> $out/share/plymouth/themes/osk-test/osk-test.plymouth <<'EOF'
        [OnScreenKeyboard]
        ${if light then "BackgroundColor=0xe6e8ec\nKeyColor=0xffffff\nActionColor=0xc4cbd4\nTextColor=0x18202a\nSelectedColor=0x87bfff" else "BackgroundColor=0x23262e\nKeyColor=0x0e141b"}
        EOF
      '') ];
    };
    virtualisation = {
      emptyDiskImages = [ 512 ];
      useBootLoader = true;
      mountHostNixStore = true;
      useEFIBoot = true;
      qemu.options = (if rotate then [ "-vga none" "-device virtio-vga,xres=800,yres=1280" ] else if scale == 2 then [ "-vga none" "-device virtio-vga,xres=1920,yres=1080" ] else [ "-vga virtio" ])
        ++ lib.optionals multi [ "-machine q35" "-device pcie-root-port,id=osk-port,bus=pcie.0,chassis=7,slot=7" "-device virtio-gpu-pci,id=osk-external,bus=osk-port,xres=1024,yres=768" ];
    };
    boot.loader.systemd-boot.enable = true;
    boot.initrd.kernelModules = [ "virtio_gpu" "uinput" ];
    boot.kernelParams = [ "console=tty0" "plymouth.ignore-serial-consoles"
      "plymouth.debug=stream:/run/plymouth-test.log" "plymouth.debug-key-events" ]
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
        plymouth-start.environment.PLYMOUTH_FORCE_SCALE = toString scale;
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
          # QEMU virtio GPUs cannot be PCI-hotplugged. Start the secondary
          # GPU unbound, then exercise DRM removal/recreation through sysfs.
          preStart = lib.optionalString multi ''
            echo 0000:01:00.0 > /sys/bus/pci/drivers/virtio-pci/unbind
          '';
          serviceConfig.Type = "exec";
          serviceConfig.ExecStart = "${input}/bin/test-input " + (if rotate then "keyboard" else "touchscreen");
        };
      };
    };
  };
  testScript = { nodes, ... }: builtins.replaceStrings
    [ "@ROTATED@" "@SYSTEM@" "@LAYOUT@" "@LIGHT@" "# @DISPLAY_TESTS@" "@SCALE@" ]
    [ (if rotate then "True" else "False") "${nodes.machine.specialisation.encrypted.configuration.system.build.toplevel}" layout (if light then "True" else "False") (if multi then builtins.readFile ./displays.py else "") (toString scale) ]
    (builtins.readFile ./test.py);
}
