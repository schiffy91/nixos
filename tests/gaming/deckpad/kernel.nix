# Every link against a real kernel, with no hardware:
#   - the network: one machine serves, another pairs, attaches through the
#     encrypted tunnel and its vhci_hcd presents the controller;
#   - the USB-C cable: the gadget sits on dummy_hcd, a device controller wired
#     to a host controller in the same kernel, which enumerates it;
#   - Bluetooth: BlueZ's btvirt gives the machine two controllers, one the
#     controller's and one a host's, under the real stack and daemon;
#   - the Deck's own pad: an emulated Deck controller is attached to the
#     serving machine, which takes it from the kernel, reads it and gives
#     it back. On the PC, SDL's own Steam Controller driver reads what the
#     pad holds through the whole chain and rumbles it.
# It boots VMs, so it stays out of the gaming check farm; run it with
#   nix build --impure --expr 'let f = builtins.getFlake (toString ./.); in import ./tests/gaming/deckpad/kernel.nix { pkgs = import f.inputs.nixpkgs { system = "x86_64-linux"; }; inherit (f) inputs; }'
{ pkgs, inputs }:
let
  programs = import ../../../config/pkgs/gaming/deckpad/package.nix { inherit pkgs inputs; };
  deckpad = "${programs.deckpad}/bin/deckpad";
  # nixpkgs builds BlueZ without its controller emulator.
  bluezVirtual = pkgs.bluez.overrideAttrs (old: {
    configureFlags = old.configureFlags ++ [ "--enable-testing" ];
    doCheck = false;
    postInstall = old.postInstall + ''
      install -Dm755 emulator/btvirt $out/bin/btvirt
    '';
  });
  tests = (import ../../../config/lib/btrc.nix { inherit pkgs inputs; }) {
    name = "deckpad-bluetooth-host";
    src = pkgs.lib.fileset.toSource {
      root = ../../..;
      fileset = pkgs.lib.fileset.unions [
        ./BluetoothHostTests.btrc ./FakeDeck.btrc ./btrc.toml ./btrc.lock ../../lib/Check.btrc
        (pkgs.lib.fileset.fileFilter (file: file.hasExt "btrc" || file.name == "btrc.toml" || file.name == "btrc.lock") ../../../config/pkgs/gaming/deckpad)
      ];
    };
    programs.deckpad-bluetooth-host = "tests/gaming/deckpad/BluetoothHostTests.btrc";
    programs.deckpad-fake-deck = "tests/gaming/deckpad/FakeDeck.btrc";
    buildInputs = [ pkgs.dbus pkgs.libsodium ];
    env.NIX_CFLAGS_COMPILE = "-I${pkgs.lib.getDev pkgs.dbus}/include/dbus-1.0 -I${pkgs.lib.getLib pkgs.dbus}/lib/dbus-1.0/include";
    linkFlags = [ "-ldbus-1" "-lsodium" ];
  };
  # A host that shares no code with deckpad: SDL's Steam Controller driver.
  sdlHost = pkgs.runCommandCC "deckpad-sdl-host" { buildInputs = [ pkgs.sdl3 ]; } ''
    mkdir -p $out/bin
    $CC -O2 -Wall -Wextra -o $out/bin/deckpad-sdl-host ${./SdlHost.c} -lSDL3 -lm
  '';
  # One service per link, so each is exercised alone.
  service = name: arguments: {
    serviceConfig = {
      ExecStart = "${deckpad} serve --source demo ${arguments} --runtime /run/${name} --state /var/lib/${name}";
      ExecStopPost = "${deckpad} release";
      RuntimeDirectory = name;
      StateDirectory = name;
    };
  };
in pkgs.testers.runNixOSTest {
  name = "deckpad-kernel";
  nodes.deck = {
    networking.firewall.allowedTCPPorts = [ 3240 ];
    services.openssh.enable = true;   # for the host key, the machine's identity
    boot.kernelModules = [ "dummy_hcd" "libcomposite" "usb_f_fs" "hci_vhci" "vhci-hcd" ];
    hardware.bluetooth = {
      enable = true;
      package = bluezVirtual;
      disabledPlugins = [ "input" ];
      settings.General = { Class = "0x002508"; DeviceID = "usb:28DE:1303:0100"; };
    };
    environment.systemPackages = [ programs.deckpad bluezVirtual ];
    systemd.services.btvirt.serviceConfig.ExecStart = "${bluezVirtual}/bin/btvirt -l2";
    systemd.services.deckpad = service "deckpad" "--serial FXDK00000001";
    systemd.services.deckpad-usb = service "deckpad-usb" "--no-network --usb --serial FXDK00000002 --control-port 3242";
    systemd.services.deckpad-bluetooth = service "deckpad-bluetooth" "--no-network --bluetooth hci0 --serial FXDK00000003 --control-port 3243";
    # The emulated Deck controller, and a service that reads it. The service
    # shares the first one's state, so the PC it paired with is still trusted.
    systemd.services.fake-deck.serviceConfig = {
      ExecStart = "${tests}/bin/deckpad-fake-deck /run/fake-deck/commands";
      RuntimeDirectory = "fake-deck";
    };
    systemd.services.deckpad-deck.serviceConfig = {
      ExecStart = "${deckpad} serve --source deck --serial FXDK00000004 --control-port 3244 --runtime /run/deckpad-deck --state /var/lib/deckpad";
      ExecStopPost = "${deckpad} release";
      RuntimeDirectory = "deckpad-deck";
    };
  };
  nodes.pc = {
    services.openssh.enable = true;
    boot.kernelModules = [ "vhci-hcd" ];
    environment.systemPackages = [ programs.deckpad ];
    systemd.services.deckpad-attach.serviceConfig = {
      ExecStart = "${deckpad} attach deck --watch";
      ExecStopPost = "${deckpad} detach";
      StateDirectory = "deckpad";
      Restart = "always";
      RestartSec = 1;
    };
  };
  testScript = ''
    def controller_present(machine, serial):
        return f"grep -qx {serial} /sys/bus/usb/devices/*/serial"

    def reads_reports(machine):
        # Reports flow on the hidraw node, the one Steam reads. Each read
        # returns one report; its first byte is the 0x42 state ID, or 0x43
        # for the battery.
        machine.wait_until_succeeds("ls /sys/bus/hid/devices | grep -qi ':28DE:1302\\.'", timeout=120)
        node = machine.wait_until_succeeds("basename /sys/bus/hid/devices/*28DE:1302*/hidraw/hidraw*", timeout=120).strip()
        first = machine.succeed(f"for turn in 1 2 3 4 5 6; do timeout 10 head -c 1 /dev/{node} | od -An -tx1; done")
        assert first.split().count("42") >= 4 and set(first.split()) <= {"42", "43"}, first

    start_all()
    deck.wait_for_unit("multi-user.target")
    pc.wait_for_unit("multi-user.target")

    with subtest("each machine is known by its SSH host key"):
        assert deck.succeed("deckpad key").split()[:2] == deck.succeed("cat /etc/ssh/ssh_host_ed25519_key.pub").split()[:2]

    with subtest("the network: pairing, then an encrypted attach through vhci_hcd"):
        deck.succeed("systemctl start deckpad.service")
        deck.wait_for_open_port(3240)
        pc.fail("deckpad attach deck")
        pc.fail("deckpad pair deck --yes")
        deck.succeed("deckpad pairing 120")
        paired = pc.succeed("deckpad pair deck --yes")
        code = [line.split()[-1] for line in paired.splitlines() if line.startswith("Pairing code:")][0]
        status = deck.succeed("deckpad status")
        assert f'"code": "{code}"' in status and '"label": "pc"' in status, status
        deck.succeed("deckpad pairing off")
        pc.succeed("systemctl start deckpad-attach.service")
        pc.wait_until_succeeds(controller_present(pc, "FXDK00000001"), timeout=120)
        pc.succeed("grep -qx 28de /sys/bus/usb/devices/*/idVendor && grep -qx 1302 /sys/bus/usb/devices/*/idProduct")
        reads_reports(pc)
        assert '"holder": "network"' in deck.succeed("deckpad status")

    with subtest("the network: the host comes back by itself when the controller restarts"):
        deck.succeed("systemctl restart deckpad.service")
        pc.wait_until_fails(controller_present(pc, "FXDK00000001"), timeout=60)
        pc.wait_until_succeeds(controller_present(pc, "FXDK00000001"), timeout=120)
        reads_reports(pc)
        pc.succeed("systemctl stop deckpad-attach.service")
        pc.wait_until_fails(controller_present(pc, "FXDK00000001"), timeout=60)
        deck.succeed("systemctl stop deckpad.service")

    with subtest("the Deck's pad: taken from the kernel, read, and given back"):
        deck.succeed("systemctl start fake-deck.service")
        deck.wait_until_succeeds("grep -qx FAKEDECK0001 /sys/bus/usb/devices/*/serial", timeout=120)
        pad = deck.succeed("basename $(dirname $(grep -lx FAKEDECK0001 /sys/bus/usb/devices/*/serial))").strip()
        driver = f"basename $(readlink /sys/bus/usb/devices/{pad}:1.2/driver)"
        deck.wait_until_succeeds(f"test $({driver}) = usbhid", timeout=60)
        # The hardware check reads the pad as the service does and gives it
        # back; dummy_hcd is the device controller it finds for the cable.
        checked = deck.succeed("deckpad check 3")
        print(checked)
        assert "pad:        PASS" in checked and "usb cable:  ready" in checked, checked
        deck.wait_until_succeeds(f"test $({driver}) = usbhid", timeout=60)
        deck.succeed(": > /run/fake-deck/commands")
        deck.succeed("systemctl start deckpad-deck.service")
        deck.wait_for_open_port(3240)
        pc.succeed("systemctl start deckpad-attach.service")
        pc.wait_until_succeeds(controller_present(pc, "FXDK00000004"), timeout=120)
        # While a host holds the controller the pad's interface is deckpad's,
        # not the kernel driver's, and the pad was told to be a plain gamepad.
        deck.wait_until_succeeds(f"test $({driver}) = usbfs", timeout=60)
        deck.wait_until_succeeds("grep -q 'interface 2 command 81' /run/fake-deck/commands && grep -q 'interface 2 command 87' /run/fake-deck/commands", timeout=60)
        # The pad's A, d-pad up and left pad touch arrive on the PC as the
        # controller's.
        pc.wait_until_succeeds("ls /sys/bus/hid/devices | grep -qi ':28DE:1302\\.'", timeout=120)
        node = pc.wait_until_succeeds("basename /sys/bus/hid/devices/*28DE:1302*/hidraw/hidraw*", timeout=120).strip()
        pc.wait_until_succeeds(f"timeout 10 head -c 54 /dev/{node} | od -An -tx1 -w54 | grep -q '^ 42 .. 01 20 00 02'", timeout=60)
        # The PC reads the controller as Steam does: no report is lost, they
        # keep the pad's pace and none is held up on the way.
        measured = pc.succeed("deckpad measure 5")
        print(measured)
        assert "controller: PASS" in measured and ", 0 lost," in measured, measured
        # SDL's Steam Controller driver, which shares no code with deckpad,
        # takes the controller and sees what the Deck's pad holds: buttons,
        # stick, trigger, pad touch, motion and battery. Its rumble reaches
        # the pad as the Deck's rumble command.
        deck.fail("grep -q 'interface 2 command eb' /run/fake-deck/commands")
        status, seen = pc.execute("${sdlHost}/bin/deckpad-sdl-host 30 2>&1")
        print(seen)
        assert status == 0 and seen.endswith("PASS: SDL read the Deck as a Steam Controller\n"), seen
        deck.wait_until_succeeds("grep -q 'interface 2 command eb' /run/fake-deck/commands", timeout=30)
        # The pad is the service's now. The check says so and leaves it
        # there: the service keeps serving and the PC loses no report.
        pc.succeed("(deckpad measure 8 > /tmp/measured 2>&1; echo finished >> /tmp/measured) > /dev/null 2>&1 &")
        assert "is the service serving a host" in deck.fail("deckpad check 1")
        deck.succeed("systemctl is-active deckpad-deck.service")
        pc.wait_until_succeeds("grep -qx finished /tmp/measured", timeout=60)
        measured = pc.succeed("cat /tmp/measured")
        print(measured)
        assert "controller: PASS" in measured and ", 0 lost," in measured, measured
        # Reading the pad does not keep cancelling transfers that are about
        # to complete. Counted over five seconds of steady reading: a short
        # wait cancelled about fifty a second.
        cancelled = int(deck.succeed("dmesg | grep -c 'vhci_hcd: unlink' || true").strip())
        deck.sleep(5)
        after = int(deck.succeed("dmesg | grep -c 'vhci_hcd: unlink' || true").strip())
        assert after - cancelled < 25, f"{after - cancelled} cancelled transfers in five seconds of reading the pad"
        # The host leaves: the kernel's driver gets the pad back.
        pc.succeed("systemctl stop deckpad-attach.service")
        deck.wait_until_succeeds(f"test $({driver}) = usbhid", timeout=60)
        deck.succeed("systemctl stop deckpad-deck.service fake-deck.service")
        deck.wait_until_fails("grep -qx FAKEDECK0001 /sys/bus/usb/devices/*/serial", timeout=60)

    with subtest("the USB-C cable: the host on the cable sees a wired controller"):
        deck.succeed("systemctl start deckpad-usb.service")
        deck.wait_until_succeeds(controller_present(deck, "FXDK00000002"), timeout=120)
        reads_reports(deck)
        assert '"holder": "usb"' in deck.succeed("deckpad status --control-port 3242 --runtime /run/deckpad-usb")
        # A crash leaves the port usable: the unit's release step clears the gadget.
        deck.succeed("systemctl kill --kill-whom=main -s KILL deckpad-usb.service")
        deck.wait_until_fails(controller_present(deck, "FXDK00000002"), timeout=60)
        deck.wait_until_succeeds("test ! -e /sys/kernel/config/usb_gadget/deckpad", timeout=60)

    with subtest("Bluetooth: a host pairs while pairing is open and reads the controller"):
        # The emulator creates its two controllers one after the other. The
        # daemon is started only once both exist, and powers them on; a
        # controller whose first power-on raced the emulator is powered on again.
        deck.succeed("systemctl stop bluetooth.service; systemctl start btvirt.service")
        deck.wait_until_succeeds("test -e /sys/class/bluetooth/hci0 && test -e /sys/class/bluetooth/hci1", timeout=120)
        deck.succeed("systemctl start bluetooth.service")
        deck.wait_until_succeeds(
            "btmgmt --index 0 power on; btmgmt --index 1 power on; test $(btmgmt info | grep -c 'addr 00:AA:01') = 2",
            timeout=180,
        )
        deck.succeed("systemctl start deckpad-bluetooth.service")
        deck.wait_until_succeeds("test -e /run/deckpad-bluetooth/token", timeout=60)
        options = "--control-port 3243 --runtime /run/deckpad-bluetooth"
        deck.wait_until_succeeds(f"deckpad status {options} | grep -q '\"name\": \"bluetooth\"'", timeout=60)
        deck.succeed(f"deckpad pairing 300 {options}")
        deck.wait_until_succeeds(f"deckpad status {options} | grep -q discoverable", timeout=60)
        status, result = deck.execute("${tests}/bin/deckpad-bluetooth-host hci0 hci1 2>&1")
        if status != 0:
            print(deck.succeed("btmgmt info; journalctl -u bluetooth -u deckpad-bluetooth --no-pager | tail -60"))
        assert status == 0 and result.endswith("PASS: deckpad paired and served over Bluetooth\n"), result
        deck.succeed("journalctl -u deckpad-bluetooth --no-pager | grep -q 'attached over Bluetooth'")
        deck.succeed("systemctl stop deckpad-bluetooth.service")
        # With the service stopped the check finds the adapter and its HID
        # channels free. There is no pad on this machine any more.
        checked = deck.fail("deckpad check 1")
        print(checked)
        assert "bluetooth:  ready  adapter 00:AA:01" in checked, checked
  '';
}
