#!/usr/bin/env python3
"""Assert the panel shows a graphical unlock screen, not a text console.

A Linux text console is a near-pure-black field with sparse glyph pixels; a
graphical agent (unl0kr or plymouth) paints a palette. The Deck's whole unlock
story is that a thumb can type the passphrase, so a text prompt is a failure.
"""
import sys

path = sys.argv[1]
with open(path, "rb") as handle:
    if handle.readline().strip() != b"P6":
        sys.exit(f"FAIL: {path} is not a binary PPM")
    dimensions = handle.readline().split()
    handle.readline()
    width, height = int(dimensions[0]), int(dimensions[1])
    pixels = handle.read()

total = width * height
black = sum(1 for i in range(0, min(len(pixels), total * 3) - 2, 3)
            if pixels[i] < 16 and pixels[i + 1] < 16 and pixels[i + 2] < 16)
fraction = black / total
print(f"panel {width}x{height} near-black={fraction:.3f}")
if fraction > 0.9:
    sys.exit("FAIL: the panel is a text console, so no graphical agent owns the unlock screen")
print("panel-graphical")
