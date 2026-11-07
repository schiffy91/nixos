# This fragment runs inside the same real LUKS prompt as the basic input tests.
# A second GPU is unbound/rebound in the guest: QEMU disables PCI hotplug
# for virtio-gpu. This still exercises real DRM/udev removal and recreation.
def qmp(command, arguments=None):
    client = machine.qmp_client
    assert client is not None, "The VM monitor must be connected"
    return client.send(command, {} if arguments is None else arguments)


def attach_monitor():
    machine.succeed("echo 0000:01:00.0 > /sys/bus/pci/drivers/virtio-pci/bind")
    machine.wait_until_succeeds("set -- /sys/class/drm/card*-Virtual-*; test $# -eq 2", timeout=20)
    outputs = set(machine.succeed("for output in /sys/class/drm/card*-Virtual-*; do basename \"$output\"; done").split())
    assert connector in outputs and len(outputs) == 2
    return (outputs - {connector}).pop()


def detach_monitor():
    machine.succeed("echo 0000:01:00.0 > /sys/bus/pci/drivers/virtio-pci/unbind")
    machine.wait_until_succeeds("set -- /sys/class/drm/card*-Virtual-*; test $# -eq 1", timeout=20)


def secondary_screenshot(name):
    ppm = Path(machine.out_dir) / f"{name}.ppm"
    qmp("screendump", {"filename": str(ppm), "device": "osk-external"})
    img = Image.open(ppm).convert("RGB")
    img.save(Path(machine.out_dir) / f"{name}.png")
    ppm.unlink()
    return img


def wait_secondary_keyboard(visible, name):
    for _ in range(30):
        img = secondary_screenshot(name)
        if has_keyboard(img) == visible:
            return img
        time.sleep(.25)
    raise AssertionError(f"External monitor keyboard visibility should be {visible}")


secondary = attach_monitor()
wait_keyboard(True, "primary-with-external-monitor")
wait_secondary_keyboard(False, "external-without-touch")

# An unspecified touchscreen is ambiguous when two displays are connected.
# It must not cause a keyboard to appear on a guessed output.
map_touchscreen(None)
wait_keyboard(False, "ambiguous-unmapped-primary")
wait_secondary_keyboard(False, "ambiguous-unmapped-external")
# Kernel versions can allocate connector type IDs globally or per GPU.
# Exercise duplicate-name rejection when exposed; otherwise verify that a
# plain unique connector name selects only its own display.
secondary_name = secondary.split("-", 1)[1]
map_touchscreen(secondary_name)
wait_keyboard(False, "connector-name-primary")
wait_secondary_keyboard(secondary_name != connector.split("-", 1)[1], "connector-name-external")

# Move the touchscreen association to the external monitor. The first monitor
# becomes an ordinary display; input and rendering follow the explicit target.
map_touchscreen(secondary)
wait_keyboard(False, "primary-without-touch")
external_empty = wait_secondary_keyboard(True, "external-with-touch")
primary_size = screen_width, screen_height
screen_width, screen_height = external_empty.size

# Two independently mapped touchscreens may coexist. A release from the second
# device must not release the first device's held key, even at the same slot.
map_touchscreen(connector, second=True)
wait_keyboard(True, "two-touchscreens-primary")
wait_secondary_keyboard(True, "two-touchscreens-external")
entry_region = (int(external_empty.width * .25), int(external_empty.height * .1),
                int(external_empty.width * .75), int(external_empty.height * .3))
command("down " + position(*key_position("AC01")))
command("down2 " + position(.25, .7))
command("up2")
held = secondary_screenshot("other-device-release-ignored")
assert not ImageChops.difference(external_empty.crop(entry_region), held.crop(entry_region)).getbbox()
command("up")
typed = secondary_screenshot("correct-device-release-types")
assert ImageChops.difference(external_empty.crop(entry_region), typed.crop(entry_region)).getbbox()
key("BKSP")
command("touch2-off")
wait_keyboard(False, "primary-touchscreen-detached")
wait_secondary_keyboard(True, "external-touchscreen-still-active")
command("down " + position(*key_position("AC01")))
detach_monitor()
command("up")
wait_keyboard(False, "mapped-monitor-removed")
machine.succeed("plymouth --ping")

# Reconnect, then remove the touchscreen during another held key. Both device
# removal paths must cancel selection, and returning devices must be usable.
secondary = attach_monitor()
map_touchscreen(secondary)
wait_secondary_keyboard(True, "mapped-monitor-returned")
command("down " + position(*key_position("AC01")))
command("touch-off")
wait_secondary_keyboard(False, "external-touchscreen-removed")
command("touch-on")
returned = wait_secondary_keyboard(True, "external-touchscreen-returned")
entry_region = (int(returned.width * .25), int(returned.height * .1),
                int(returned.width * .75), int(returned.height * .3))
assert not ImageChops.difference(external_empty.crop(entry_region), returned.crop(entry_region)).getbbox(), "A cancelled touch typed into the password"
detach_monitor()
screen_width, screen_height = primary_size
map_touchscreen(connector)
returned = wait_keyboard(True, "primary-restored-after-monitor-removal")
assert not ImageChops.difference(empty.crop(box), returned.crop(box)).getbbox(), "A stale external touch typed on the primary monitor"
machine.succeed("plymouth --ping")

# Temporarily remove the last display as well. The daemon and pending prompt
# must survive a completely headless interval; a held touch must be cancelled.
primary_card = connector.split("-", 1)[0]
primary_device = machine.succeed("basename $(readlink -f /sys/class/drm/" + primary_card + "/device)").strip()
key("AC01")
partial_entry = screenshot("partial-entry-before-headless-interval")
command("down " + position(*key_position("AC02")))
machine.succeed("echo " + shlex.quote(primary_device) + " > /sys/bus/pci/drivers/virtio-pci/unbind")
machine.wait_until_succeeds("test ! -e /sys/class/drm/" + connector, timeout=20)
command("up")
machine.succeed("plymouth --ping")
machine.succeed("echo " + shlex.quote(primary_device) + " > /sys/bus/pci/drivers/virtio-pci/bind")
machine.wait_until_succeeds("set -- /sys/class/drm/card*-Virtual-*; test -e \"$1\"", timeout=20)
connector = machine.succeed("for output in /sys/class/drm/card*-Virtual-*; do basename \"$output\"; done").strip()
assert "\n" not in connector
map_touchscreen(connector)
returned = wait_keyboard(True, "display-returned-after-headless-interval")
assert not ImageChops.difference(partial_entry.crop(box), returned.crop(box)).getbbox(), "Display removal changed the password or committed a held touch"
key("BKSP")
cleared = screenshot("headless-entry-correction")
assert not ImageChops.difference(empty.crop(box), cleared.crop(box)).getbbox()
