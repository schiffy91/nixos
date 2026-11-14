#!/usr/bin/env python3
"""icons.py MARK.svg OUT: the tray's pictures, one a state, drawn over the Drive's mark."""
import math
import re
import sys
from pathlib import Path


class Icons:
    SIZE = 64
    BADGE = (46.0, 46.0, 17.0)  # centre and radius of the corner disc

    @classmethod
    def mark(cls, source, opacity=1.0):
        body = re.sub(r"^.*?<svg[^>]*>|</svg>\s*$", "", Path(source).read_text(), flags=re.S)
        return f'<g opacity="{opacity}">{body}</g>'

    @classmethod
    def disc(cls, fill):
        x, y, r = cls.BADGE
        return f'<circle cx="{x}" cy="{y}" r="{r}" fill="#ffffff"/><circle cx="{x}" cy="{y}" r="{r - 2.5}" fill="{fill}"/>'

    @classmethod
    def pie(cls, percent):
        x, y, r = cls.BADGE
        r -= 2.5
        if percent >= 100:
            return f'<circle cx="{x}" cy="{y}" r="{r}" fill="#1a73e8"/>'
        if percent <= 0:
            return ""
        angle = 2 * math.pi * percent / 100
        end = (x + r * math.sin(angle), y - r * math.cos(angle))
        return f'<path d="M {x} {y} L {x} {y - r} A {r} {r} 0 {1 if percent > 50 else 0} 1 {end[0]:.3f} {end[1]:.3f} Z" fill="#1a73e8"/>'

    @classmethod
    def arrows(cls):  # two arcs chasing each other: at work, how far along unknown
        x, y, r = cls.BADGE
        r -= 7.5
        arcs = []
        for turn in (0, 180):
            arcs.append(
                f'<g transform="rotate({turn} {x} {y})"><path d="M {x - r} {y} A {r} {r} 0 0 1 {x + r * 0.6:.3f} {y - r * 0.8:.3f}" fill="none" stroke="#ffffff" stroke-width="3.2" stroke-linecap="round"/>'
                f'<path d="M {x + r * 0.6 - 4.5:.3f} {y - r * 0.8 - 5:.3f} L {x + r * 0.6 + 3.5:.3f} {y - r * 0.8 - 0.5:.3f} L {x + r * 0.6 - 3:.3f} {y - r * 0.8 + 4.5:.3f} Z" fill="#ffffff"/></g>'
            )
        return "".join(arcs)

    @classmethod
    def bars(cls):
        x, y, _ = cls.BADGE
        return "".join(f'<rect x="{x + shift}" y="{y - 7}" width="4.5" height="14" rx="1" fill="#ffffff"/>' for shift in (-6.5, 2))

    @classmethod
    def exclamation(cls):
        x, y, _ = cls.BADGE
        return f'<rect x="{x - 2.4}" y="{y - 9}" width="4.8" height="11" rx="1.5" fill="#ffffff"/><circle cx="{x}" cy="{y + 6.5}" r="2.7" fill="#ffffff"/>'

    @classmethod
    def write(cls, out, name, *parts):
        (out / f"{name}.svg").write_text(f'<svg xmlns="http://www.w3.org/2000/svg" width="{cls.SIZE}" height="{cls.SIZE}" viewBox="0 0 {cls.SIZE} {cls.SIZE}">{"".join(parts)}</svg>\n')

    @classmethod
    def main(cls, source, out):
        out = Path(out)
        out.mkdir(parents=True, exist_ok=True)
        cls.write(out, "synced", cls.mark(source))
        cls.write(out, "syncing", cls.mark(source), cls.disc("#1a73e8"), cls.arrows())
        for percent in range(0, 101, 5):
            cls.write(out, f"syncing-{percent:03d}", cls.mark(source), cls.disc("#c6dafc"), cls.pie(percent))
        cls.write(out, "paused", cls.mark(source), cls.disc("#5f6368"), cls.bars())
        cls.write(out, "offline", cls.mark(source, 0.35))
        cls.write(out, "attention", cls.mark(source), cls.disc("#e37400"), cls.exclamation())


if __name__ == "__main__":
    Icons.main(*sys.argv[1:3])
