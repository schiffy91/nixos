#!/usr/bin/env nix-shell
#!nix-shell -i python3 -p python3 imagemagick curl
"""Compose the Steam artwork for the "Deck Pad" shortcut from Valve's Steam Controller pictures."""
import subprocess, sys, tempfile
from pathlib import Path

CLAN = "https://clan.akamai.steamstatic.com/images/clan/45479024"
SOURCES = {  # store.steampowered.com/hardware/steamcontroller, November 2026
    "paper.avif": f"{CLAN}/e382ec72f1ee14645c4545f2f1af3106.poster.avif",  # the controller on kraft paper, 1920x1080
    "product.jpg": "https://shared.akamai.steamstatic.com/store_item_assets/steam/subs/1558609/b3f962ed3d8fc379da42acf89fcf88cac2583461/capsule_616x353_2x.jpg",  # front view on white, 1232x706
    "glyph.avif": f"{CLAN}/02e9a2b16890a5c18fedeab2ddca7ef1.avif",  # Valve's controller glyph, 1025x813
}
INK = "#17191d"

class Artwork:
    def __init__(self, out, work):
        self.out, self.work = Path(out), Path(work)
        self.out.mkdir(parents=True, exist_ok=True)
        font = subprocess.check_output(["nix", "build", "--no-link", "--print-out-paths", "nixpkgs#montserrat"], text=True).split()[0]
        self.font = f"{font}/share/fonts/otf/Montserrat-ExtraBold.otf"

    def magick(self, *arguments):
        subprocess.run(["magick", *map(str, arguments)], check=True, cwd=self.work)

    def fetch(self):
        for name, url in SOURCES.items():
            if not (self.work / name).exists():
                subprocess.run(["curl", "-sSfL", "-o", str(self.work / name), url], check=True)

    def parts(self):
        # The controller cut out of its white background, with a feathered edge and a soft shadow to sit on paper.
        self.magick("product.jpg", "-fuzz", "3%", "-trim", "+repage", "-bordercolor", "white", "-border", "60", "product.png")
        self.magick("product.png", "-colorspace", "gray", "-threshold", "95%", "-negate", "-morphology", "erode", "disk:1.5", "-blur", "0x0.8", "mask.png")
        self.magick("product.png", "mask.png", "-alpha", "off", "-compose", "CopyOpacity", "-composite", "controller.png")
        self.magick("mask.png", "-blur", "0x22", "-evaluate", "multiply", "0.55", "(", "+clone", "-fill", "#2a1a08", "-colorize", "100", ")", "+swap", "-alpha", "off", "-compose", "CopyOpacity", "-composite", "shadow.png")
        self.magick("glyph.avif", "-crop", "540x400+244+248", "+repage", "glyph.png")
        # Clean kraft paper: the left of Valve's picture, mirrored where a wider piece is needed.
        self.magick("paper.avif", "-crop", "660x1000+20+60", "+repage", "paper.png")

    def controller(self, width, x, y):
        return ["(", "shadow.png", "-resize", f"{width}x", ")", "-geometry", f"+{x}+{y + round(width * 0.035)}", "-compose", "over", "-composite",
                "(", "controller.png", "-resize", f"{width}x", ")", "-geometry", f"+{x}+{y}", "-compose", "over", "-composite"]

    def text(self, words, size, color, gravity, x, y, tracking=0):
        return ["-font", self.font, "-pointsize", size, "-kerning", tracking, "-fill", color, "-gravity", gravity, "-annotate", f"+{x}+{y}", words, "-gravity", "NorthWest"]

    def portrait(self):
        self.magick("paper.png", "-gravity", "center", "-crop", "600x900+0+0", "+repage", "-gravity", "NorthWest",
                    *self.controller(540, 30, 180),
                    *self.text("DECK PAD", 84, INK, "North", 0, 650, 3),
                    self.out / "p.png")

    def landscape(self):
        self.magick("(", "paper.png", "-crop", "460x430+100+300", "+repage", ")", "(", "+clone", "-flop", ")", "+append", "-gravity", "NorthWest",
                    *self.controller(420, 26, 50),
                    *self.text("DECK PAD", 64, INK, "NorthWest", 486, 166, 3),
                    self.out / "landscape.png")

    def hero(self):
        self.magick("paper.avif", "-crop", "1920x620+0+255", "+repage", self.out / "_hero.png")

    def logo(self):
        self.magick("-size", "1500x260", "xc:none",
                    "(", "glyph.png", "-fill", "white", "-colorize", "100", "-resize", "x190", ")", "-geometry", "+20+35", "-composite",
                    *self.text("DECK PAD", 92, "white", "West", 305, 0, 4),
                    "(", "+clone", "-background", "black", "-shadow", "55x7+0+4", ")", "+swap", "-background", "none", "-layers", "merge", "+repage", "-trim", "+repage",
                    self.out / "_logo.png")

    def icon(self):
        self.magick("(", "paper.png", "-crop", "512x512+70+240", "+repage", ")",
                    "(", "glyph.png", "-resize", "400x", ")", "-gravity", "center", "-composite",
                    "(", "-size", "512x512", "xc:black", "-fill", "white", "-draw", "roundrectangle 0,0 511,511 112,112", ")", "-alpha", "off", "-compose", "CopyOpacity", "-composite",
                    self.out / "icon.png")

    def compose(self):
        self.fetch()
        self.parts()
        for piece in (self.portrait, self.landscape, self.hero, self.logo, self.icon):
            piece()

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else Path(__file__).parent
    work = sys.argv[2] if len(sys.argv) > 2 else tempfile.mkdtemp()
    Artwork(out, work).compose()
