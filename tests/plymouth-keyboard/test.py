import shlex
import time
from pathlib import Path
from PIL import Image, ImageChops

rotated = @ROTATED@
layout = "@LAYOUT@"
light = @LIGHT@
scale = @SCALE@
passphrase = {"us": "aB3! z", "de": "äÖz@ €", "fr": "éAê €"}[layout]
machine.wait_for_unit("multi-user.target")
# Public test credentials, written without a shell pipeline or newline.
machine.succeed("install -m 600 /dev/null /tmp/test-key")
machine.succeed("printf %s " + shlex.quote(passphrase) + " > /tmp/test-key")
machine.succeed("cryptsetup luksFormat -q --iter-time=1 --key-file /tmp/test-key /dev/vdb")
machine.succeed("cryptsetup luksOpen --key-file /tmp/test-key /dev/vdb cryptroot")
machine.succeed("rm /tmp/test-key; mkfs.ext4 /dev/mapper/cryptroot")
machine.succeed("@SYSTEM@/bin/switch-to-configuration boot")
machine.succeed("sync")
machine.crash()
machine.start()
machine.wait_for_unit("plymouth-start.service", timeout=90)
machine.wait_for_unit("test-input.service", timeout=15)
machine.succeed("test -s /run/request-before-drivers")
machine.wait_until_succeeds("test -p /run/test-input", timeout=15)
machine.wait_until_succeeds("plymouth --has-active-vt")
time.sleep(3)


def command(value):
    machine.succeed("echo " + shlex.quote(value) + " > /run/test-input")
    time.sleep(0.15)


def screenshot(name):
    machine.screenshot(name)
    img = Image.open(Path(machine.out_dir) / f"{name}.png").convert("RGB")
    if rotated:
        assert img.width < img.height, "Must emulate a native portrait panel"
        img = img.transpose(Image.Transpose.ROTATE_90)
        img.save(Path(machine.out_dir) / f"{name}-upright.png")
    if scale != 1:
        img = img.resize((img.width // scale, img.height // scale), Image.Resampling.NEAREST)
        img.save(Path(machine.out_dir) / f"{name}-logical.png")
    return img


def has_keyboard(img):
    tray = (0xe6, 0xe8, 0xec) if light else (0x23, 0x26, 0x2e)
    key_color = (255, 255, 255) if light else (0x0e, 0x14, 0x1b)
    # Read backgrounds away from labels and borders; never inspect password text.
    colors = img.crop((0, img.height // 2, img.width, img.height)).getcolors(img.width * img.height)
    counts = {color: count for count, color in colors}
    return counts.get(tray, 0) > img.width * 10 and counts.get(key_color, 0) > img.width * 10


def wait_keyboard(visible, name):
    for _ in range(20):
        img = screenshot(name)
        field = img.crop((img.width // 4, 0, 3 * img.width // 4, int(img.height * .45)))
        border = sum(b > 120 and b > 2 * r and g > 60 for r, g, b in field.getdata())
        if has_keyboard(img) == visible and border > 50:
            return img
        time.sleep(.5)
    status, diagnostics = machine.execute("cat /run/plymouth-test.log")
    print(diagnostics)
    status, devices = machine.execute('for event in /dev/input/event*; do udevadm info -q property -n "$event"; done')
    print(devices)
    raise AssertionError(f"Password field missing or keyboard visibility should be {visible}: {name}")


def position(x, y):
    if rotated:
        x, y = 1 - y, x
    return f"{int(x * 32767)} {int(y * 32767)}"


def touch(x, y):
    command("down " + position(x, y))
    command("up")


# Independent ISO key-position specification, in quarter-key units.
positions = {"TLDE": (0, 0, 4), "BKSP": (52, 0, 8), "TAB": (0, 4, 4),
             "CAPS": (0, 8, 7), "LFSH": (0, 12, 6), "LSGT": (6, 12, 4),
             "RTSH": (50, 12, 10), "GROUP": (0, 16, 10), "RALT": (10, 16, 8),
             "SPCE": (18, 16, 24), "RTRN": (42, 16, 18)}
for prefix, count, first, row in [("AE", 12, 4, 0), ("AD", 12, 4, 4),
                                  ("AC", 11, 7, 8), ("AB", 10, 10, 12)]:
    for number in range(1, count + 1):
        positions[f"{prefix}{number:02}"] = (first + (number - 1) * 4, row, 4)


def key_position(name):
    x, y, width = positions[name]
    kw = min(int(screen_width * .96), 1280)
    kh = screen_height * 52 // 100
    px = (screen_width - kw) // 2 + kw * (x + width / 2) / 60
    py = screen_height - kh + 4 + (kh - 8) * (y + 2) / 20
    return px / screen_width, py / screen_height


def key(name):
    touch(*key_position(name))


if not rotated:
    wait_keyboard(True, "touchscreen-present-at-boot")
    command("touch-off")
wait_keyboard(False, "no-touchscreen")
command("touch-on")
if rotated:
    wait_keyboard(False, "physical-keyboard-attached")
    command("keyboard-off")
empty = wait_keyboard(True, "automatic-keyboard")
screen_width, screen_height = empty.size
command("keyboard-on")
hidden = wait_keyboard(False, "keyboard-hotplug-hidden")
command("key 30")
typed = screenshot("physical-keyboard-typed")
field = (int(hidden.width * .25), int(hidden.height * .1), int(hidden.width * .75), int(hidden.height * .46))
assert ImageChops.difference(hidden.crop(field), typed.crop(field)).getbbox(), "Physical keyboard did not type"
command("key 14")
cleared = screenshot("physical-keyboard-cleared")
assert not ImageChops.difference(hidden.crop(field), cleared.crop(field)).getbbox()
physical_trace_count = machine.succeed("cat /run/plymouth-test.log").count("Processing input")
assert physical_trace_count > 0, "Key-event debugging must be active for the confidentiality check"
key("AC01")  # A reveal tap must not type.
wait_keyboard(True, "tap-reveals-keyboard")
touch(.5, .3)
command("down " + position(.18, .73))
command("move " + position(.5, .3))
command("up")
after = screenshot("reveal-does-not-type")
box = (int(empty.width * .25), int(empty.height * .1), int(empty.width * .75), int(empty.height * .3))
assert not ImageChops.difference(empty.crop(box), after.crop(box)).getbbox()
command("touch-off")
wait_keyboard(False, "touchscreen-removed")
command("touch-on")
wait_keyboard(True, "touchscreen-returned")

# Exercise real libinput WL_OUTPUT associations against the DRM connector.
def map_touchscreen(output, second=False):
    device = "touch2" if second else "touch"
    name = "Plymouth test touchscreen 2" if second else "Plymouth test touchscreen"
    path = "/run/udev/rules.d/99-test-touch-output" + ("-2" if second else "") + ".rules"
    command(device + "-off")
    rule = 'SUBSYSTEM=="input", KERNEL=="event*", ATTRS{name}=="' + name + '", ENV{WL_OUTPUT}="' + (output or "") + '"'
    machine.succeed("mkdir -p /run/udev/rules.d")
    if output is None:
        machine.succeed("rm -f " + path)
    else:
        machine.succeed("printf '%s\\n' " + shlex.quote(rule) + " > " + path)
    machine.succeed("udevadm control --reload")
    command(device + "-on")


# A second finger cannot release or replace the first finger's selection.
command("down " + position(*key_position("AC01")))
command("down " + position(*key_position("AC02")) + " 1")
command("up 0 0 1")
held = screenshot("other-finger-release-ignored")
assert not ImageChops.difference(empty.crop(box), held.crop(box)).getbbox()
command("up")
typed = screenshot("first-finger-release-types")
assert ImageChops.difference(empty.crop(box), typed.crop(box)).getbbox()
key("BKSP")
cleared = screenshot("touch-correction-clears")
assert not ImageChops.difference(empty.crop(box), cleared.crop(box)).getbbox()

map_touchscreen("not-a-connected-output")
wait_keyboard(False, "unmatched-touch-output")
map_touchscreen("Virtual-1")
wait_keyboard(True, "connector-touch-output")
connector = machine.succeed("for output in /sys/class/drm/card*-Virtual-1; do basename \"$output\"; done").strip()
assert "\n" not in connector and connector.endswith("-Virtual-1")
map_touchscreen(connector)
wait_keyboard(True, "card-qualified-touch-output")

@DISPLAY_TESTS@

key("GROUP")
screenshot("alternate-layout")
key("GROUP")
screenshot("configured-layout")

# A bad passphrase must return to a usable touch prompt.
key("AC01")
key("RTRN")
time.sleep(3)
wait_keyboard(True, "retry-keyboard")
machine.fail("test -b /dev/mapper/cryptroot")

if layout == "us":
    key("AC01")  # a
    key("CAPS" if rotated else "LFSH")
    key("AB05")  # B
    if rotated:
        key("CAPS")
    key("AE03")
    key("LFSH")
    key("AE01")  # !
    key("SPCE")
    key("AB02")  # erroneous x
    key("BKSP")
    key("AB01")  # z
elif layout == "de":
    key("AC11")  # ä
    key("CAPS")
    key("AC10")  # Ö
    key("CAPS")
    key("AD06")  # z
    key("RALT")
    key("AD01")  # @
    key("SPCE")
    key("RALT")
    key("AD03")  # €
else:
    key("AE02")  # é
    key("LFSH")
    key("AD01")  # A
    key("AD11")  # dead circumflex
    key("AD03")  # ê
    key("SPCE")
    key("RALT")
    key("AD03")  # €

# UTF-8 correction removes a whole character, not one byte.
if layout != "us":
    key("BKSP")
    key("RALT")
    key("AD03")
screenshot("passphrase-entered")
trace = machine.succeed("cat /run/plymouth-test.log")
assert trace.count("Processing input") == physical_trace_count, "Touch input entered raw key-event tracing"
assert passphrase not in trace, "Passphrase appeared in diagnostics"
key("RTRN")
try:
    machine.wait_until_succeeds("test -b /dev/mapper/cryptroot", timeout=30)
except Exception:
    print(machine.succeed("cat /run/plymouth-test.log"))
    print(machine.succeed("systemctl show plymouth-start.service -p Environment"))
    raise
machine.switch_root()
machine.wait_for_unit("multi-user.target")
machine.succeed("findmnt -n -o SOURCE / | grep -Fx /dev/mapper/cryptroot")
