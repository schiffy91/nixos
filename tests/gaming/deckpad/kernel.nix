# The real-kernel half of the localhost path: vhci_hcd attaches the served
# controller, usbhid enumerates it and the HID core takes its reports. It boots
# a VM, so it stays out of the gaming check farm; run it with
#   nix build --impure --expr 'let f = builtins.getFlake (toString ./.); in import ./tests/gaming/deckpad/kernel.nix { pkgs = import f.inputs.nixpkgs { system = "x86_64-linux"; }; inherit (f) inputs; }'
{ pkgs, inputs }:
let
  programs = import ../../../config/pkgs/gaming/deckpad/package.nix { inherit pkgs inputs; };
  deckpad = "${programs.deckpad}/bin/deckpad";
in pkgs.testers.runNixOSTest {
  name = "deckpad-kernel";
  nodes.machine = {
    boot.kernelModules = [ "vhci-hcd" ];
    systemd.services.deckpad = {
      wantedBy = [ "multi-user.target" ];
      serviceConfig.ExecStart = "${deckpad} serve --source demo --serial FXDK00000001";
    };
  };
  testScript = ''
    machine.wait_for_unit("deckpad.service")
    machine.wait_for_open_port(3240)
    machine.succeed("${deckpad} attach 127.0.0.1")
    # A USB device with Valve's wired Steam Controller identity appears...
    machine.wait_until_succeeds("grep -qx 1302 /sys/bus/usb/devices/*/idProduct", timeout=60)
    machine.succeed("grep -qx 28de /sys/bus/usb/devices/*/idVendor")
    machine.succeed("grep -qx FXDK00000001 /sys/bus/usb/devices/*/serial")
    # ...its one interface is HID, and the report descriptor was accepted.
    machine.wait_until_succeeds("ls /sys/bus/hid/devices | grep -qi ':28DE:1302\\.'", timeout=60)
    machine.wait_until_succeeds("ls /sys/class/hidraw | grep -q hidraw", timeout=60)
    print(machine.succeed("dmesg | grep -iE 'vhci|usb [0-9-]+:|hid|steam|input:' | tail -40"))
    print(machine.succeed("for d in /sys/bus/hid/devices/*28DE:1302*; do echo $d; readlink $d/driver; ls $d; done"))
    print(machine.succeed("cat /proc/bus/input/devices"))
    # Reports flow on its hidraw node, the one Steam reads. Each read returns one
    # report; its first byte is the 0x42 state ID, or 0x43 for the battery.
    node = machine.succeed("basename /sys/bus/hid/devices/*28DE:1302*/hidraw/hidraw*").strip()
    first = machine.succeed(f"for turn in 1 2 3 4 5 6; do timeout 10 head -c 1 /dev/{node} | od -An -tx1; done")
    assert first.split().count("42") >= 4 and set(first.split()) <= {"42", "43"}, first
    machine.succeed("${deckpad} detach | grep -q 'detached 1'")
    machine.wait_until_fails("grep -qx 1302 /sys/bus/usb/devices/*/idProduct", timeout=30)
    machine.succeed("journalctl -u deckpad.service | grep -q 'host detached'")
  '';
}
