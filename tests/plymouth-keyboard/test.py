import time
from pathlib import Path
from PIL import Image, ImageChops

rotated = @ROTATED@
machine.wait_for_unit("multi-user.target")
# Deliberately includes uppercase, punctuation, digits and a space.
machine.succeed("printf 'aB3! z' | cryptsetup luksFormat -q --iter-time=1 /dev/vdb -")
machine.succeed("printf 'aB3! z' | cryptsetup luksOpen /dev/vdb cryptroot")
machine.succeed("mkfs.ext4 /dev/mapper/cryptroot")
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
    machine.succeed(f"echo '{value}' > /run/test-input")
    time.sleep(0.15)


def screenshot(name):
    machine.screenshot(name)
    img = Image.open(Path(machine.out_dir) / f"{name}.png").convert("RGB")
    if rotated:
        assert img.width < img.height, "Must emulate the Deck's native portrait panel"
        img = img.transpose(Image.Transpose.ROTATE_90)
        img.save(Path(machine.out_dir) / f"{name}-upright.png")
    return img


def has_keyboard(img):
    # Sample the tray and a character key, away from text. The default Steam
    # theme has a charcoal tray and dark keys; Unlock is black like other actions.
    tray = img.getpixel((int(img.width * .01), int(img.height * .965)))
    key = img.getpixel((int(img.width * .045), int(img.height * .56)))
    return tray == (0x23, 0x26, 0x2e) and key == (0x0e, 0x14, 0x1b)


def wait_keyboard(visible, name):
    for _ in range(20):
        img = screenshot(name)
        # The blue border of the password field must also be present. A late
        # DRM display must restore the pending prompt, not only the keyboard.
        field = img.crop((img.width // 4, 0, 3 * img.width // 4, int(img.height * .45)))
        border = sum(b > 120 and b > 2 * r and g > 60 for r, g, b in field.getdata())
        if has_keyboard(img) == visible and border > 50:
            return img
        time.sleep(.5)
    raise AssertionError(f"Password field missing or keyboard visibility should be {visible}: {name}")


def touch(x, y):
    if rotated:
        x, y = 1 - y, x
    command(f"down {int(x * 32767)} {int(y * 32767)}")
    command("up")


def key(column, row, units=1):
    # Independent specification of the five-row US keyboard, in 15 columns.
    touch(.02 + .96 * (column + units / 2) / 15, .48 + .10 * (row + .5))


if not rotated:
    wait_keyboard(True, "touchscreen-present-at-boot")
    command("touch-off")
wait_keyboard(False, "no-touchscreen")
command("touch-on")
if rotated:
    wait_keyboard(False, "physical-keyboard-attached")
    command("keyboard-off")
empty = wait_keyboard(True, "automatic-keyboard")
command("keyboard-on")
hidden = wait_keyboard(False, "keyboard-hotplug-hidden")
command("key 30")  # Physical KEY_A, through Plymouth's ordinary evdev path.
typed = screenshot("physical-keyboard-typed")
field = (int(hidden.width * .25), int(hidden.height * .1), int(hidden.width * .75), int(hidden.height * .46))
assert ImageChops.difference(hidden.crop(field), typed.crop(field)).getbbox(), "Physical keyboard did not type"
command("key 14")  # Physical Backspace.
cleared = screenshot("physical-keyboard-cleared")
assert not ImageChops.difference(hidden.crop(field), cleared.crop(field)).getbbox()
key(2, 2)  # Tap where 'a' will appear: the reveal tap must not type it.
wait_keyboard(True, "tap-reveals-keyboard")

# Touches outside keys and a drag off a key must not type anything either.
touch(.5, .3)
def position(x, y):
    if rotated:
        x, y = 1 - y, x
    return f"{int(x * 32767)} {int(y * 32767)}"

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

# Bad passphrase must return to a usable touch prompt.
key(2, 2)  # a
key(10.5, 4, 4.5)  # Unlock
time.sleep(3)
wait_keyboard(True, "retry-keyboard")
machine.fail("test -b /dev/mapper/cryptroot")

# Typed using only touchscreen events, including correction with Backspace.
key(2, 2)  # a
key(0, 4 if rotated else 3, 2.5)  # Caps in the rotated test; Shift otherwise.
key(6.5, 3)  # B
if rotated:
    key(0, 4, 2.5)  # Caps off
key(3, 0)  # 3
key(0, 3, 2.5)  # Shift
key(1, 0)  # !
key(2.5, 4, 8)  # Space
key(3.5, 3)  # x
key(13, 0, 2)  # Backspace
key(2.5, 3)  # z
screenshot("passphrase-entered")
key(10.5, 4, 4.5)
machine.wait_until_succeeds("test -b /dev/mapper/cryptroot", timeout=60)
machine.switch_root()
machine.wait_for_unit("multi-user.target")
machine.succeed("findmnt -n -o SOURCE / | grep -Fx /dev/mapper/cryptroot")
