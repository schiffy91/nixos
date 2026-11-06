{ system ? builtins.currentSystem }:
let
  locked = (builtins.fromJSON (builtins.readFile ../../flake.lock)).nodes.nixpkgs.locked;
  pkgs = import (builtins.fetchTree {
    inherit (locked) type owner repo rev narHash;
  }) { inherit system; };
in pkgs.testers.runNixOSTest {
  name = "initrd-touch-unlock";
  extraPythonPackages = packages: [ packages.pillow ];
  nodes.machine = { lib, pkgs, ... }: {
    imports = [ ../../nix/settings.nix ../../nix/system/nixos/boot/touch-keyboard.nix ];
    settings.boot.touchKeyboard = true;
    virtualisation = {
      emptyDiskImages = [ 512 ];
      useBootLoader = true;
      mountHostNixStore = true;
      useEFIBoot = true;
      qemu.options = [ "-vga virtio" ];
    };
    boot.loader.systemd-boot.enable = true;
    boot.initrd.kernelModules = [ "virtio_gpu" ];
    environment.systemPackages = [ pkgs.cryptsetup ];

    specialisation.encrypted.configuration = {
      testing.initrdBackdoor = true;
      boot.initrd.luks.devices = lib.mkVMOverride {
        cryptroot.device = "/dev/vdb";
      };
      virtualisation.rootDevice = "/dev/mapper/cryptroot";
      boot.initrd.systemd.services = {
        systemd-modules-load.preStart = ''
          ${pkgs.coreutils}/bin/sleep 15
          # Prove a real cryptsetup request arrived while drivers were pending.
          ${pkgs.coreutils}/bin/ls /run/systemd/ask-password/ask.* > /run/request-before-drivers
        '';
        unl0kr-agent.preStart = ''
          test -s /run/request-before-drivers
          if [ ! -e /run/unlock-retried ]; then
            ${pkgs.coreutils}/bin/touch /run/unlock-retried
            exit 1
          fi
        '';
      };
    };
  };
  testScript = { nodes, ... }: ''
    import time
    from pathlib import Path
    from typing import Any
    from PIL import Image, ImageChops

    machine.wait_for_unit("multi-user.target")
    machine.succeed("printf a | cryptsetup luksFormat -q --iter-time=1 /dev/vdb -")
    machine.succeed("printf a | cryptsetup luksOpen /dev/vdb cryptroot")
    machine.succeed("mkfs.ext4 /dev/mapper/cryptroot")
    machine.succeed("${nodes.machine.specialisation.encrypted.configuration.system.build.toplevel}/bin/switch-to-configuration boot")
    machine.succeed("sync")
    machine.crash()
    machine.start()
    machine.wait_for_unit("unl0kr-agent.service", timeout=60)
    machine.succeed("test -e /run/unlock-retried")
    machine.succeed("test -s /run/request-before-drivers")
    time.sleep(3)
    machine.screenshot("keyboard")
    screen = Image.open(Path(machine.out_dir) / "keyboard.png").convert("RGB")
    width, height = screen.size
    pixels = screen.tobytes()
    dark = sum(max(pixels[i:i + 3]) < 16 for i in range(0, len(pixels), 3))
    assert dark < width * height // 2, "The keyboard is missing; the screen is mostly console black"

    def click(x, y):
        assert machine.qmp_client is not None
        press: Any = {"events": [
            {"type": "abs", "data": {"axis": "x", "value": int(x * 32767)}},
            {"type": "abs", "data": {"axis": "y", "value": int(y * 32767)}},
            {"type": "btn", "data": {"button": "left", "down": True}},
        ]}
        machine.qmp_client.send("input-send-event", press)
        time.sleep(0.2)
        release: Any = {"events": [
            {"type": "btn", "data": {"button": "left", "down": False}},
        ]}
        machine.qmp_client.send("input-send-event", release)
        time.sleep(0.5)

    # US layout, number row visible: 'a', then the checkmark. Absolute tablet
    # input exercises the on-screen keys, without sending keyboard events.
    click(0.056, 0.725)
    machine.screenshot("typed")
    typed = Image.open(Path(machine.out_dir) / "typed.png").convert("RGB")
    field = (width // 5, height // 4, 4 * width // 5, int(height * 0.44))
    assert ImageChops.difference(screen.crop(field), typed.crop(field)).getbbox(), "Click did not enter a password"
    click(0.92, 0.94)
    machine.wait_until_succeeds("test -b /dev/mapper/cryptroot", timeout=60)
    machine.switch_root()
    machine.wait_for_unit("multi-user.target")
    machine.succeed("findmnt -n -o SOURCE / | grep -Fx /dev/mapper/cryptroot")
  '';
}
