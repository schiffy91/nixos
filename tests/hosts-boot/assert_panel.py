#!/usr/bin/env python3
"""Assert the right agent owns the panel, and that no kernel log is on it.

The fixture gives the panel the same console real hardware has, so an
unquiet boot paints the kernel ring buffer across it.

unl0kr covers the screen with an on-screen keyboard, so almost no pixel is
left at the console's pure black: that is what proves it owns the panel.
Because it covers the screen it also hides anything printed earlier, so for
unl0kr the log check is not meaningful and is not claimed.

The same top band is measured again after unlock, under the agent name
console-silent, so a boot that goes loud only after the passphrase is
caught too.

plymouth's splash is nearly black, which a text console also is, so instead
the top band is measured: kernel log lines fill it with bright glyph rows,
and a quiet boot leaves it empty.
"""
import sys

path = sys.argv[1]
agent = sys.argv[2] if len(sys.argv) > 2 else "unknown"

with open(path, "rb") as handle:
    if handle.readline().strip() != b"P6":
        sys.exit(f"FAIL: {path} is not a binary PPM")
    width, height = (int(value) for value in handle.readline().split())
    handle.readline()
    pixels = handle.read()


def scan(first_row, last_row):
    black = bright = counted = 0
    for y in range(first_row, last_row):
        row = y * width * 3
        for x in range(0, width, 2):
            offset = row + x * 3
            if offset + 2 >= len(pixels):
                break
            counted += 1
            red, green, blue = pixels[offset], pixels[offset + 1], pixels[offset + 2]
            if red < 16 and green < 16 and blue < 16:
                black += 1
            if red > 90 or green > 90 or blue > 90:
                bright += 1
    return black / max(1, counted), bright / max(1, counted)

whole_black, _ = scan(0, height)
_, top_bright = scan(0, max(1, height // 5))
print(f"panel {width}x{height} agent={agent} near_black={whole_black:.4f} top_band_bright={top_bright:.4f}")

if agent == "unl0kr":
    if whole_black > 0.5:
        sys.exit(f"FAIL: {whole_black:.0%} of the panel is console black; unl0kr is not drawing its keyboard")
elif agent == "plymouth":
    if whole_black < 0.5:
        sys.exit(f"FAIL: only {whole_black:.0%} of the panel is dark; this is not plymouth's splash")
    if top_bright > 0.02:
        sys.exit(f"FAIL: the panel's top band is {top_bright:.1%} lit, which is kernel log text, not a quiet boot")
elif agent == "console-silent":
    if top_bright > 0.02:
        sys.exit(f"FAIL: the panel's top band is {top_bright:.1%} lit, which is kernel log text, not a quiet boot")
else:
    sys.exit(f"FAIL: no panel expectation defined for agent {agent}")
print("panel-graphical")
