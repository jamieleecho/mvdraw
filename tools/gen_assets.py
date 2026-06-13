#!/usr/bin/env python3
"""Generate mvdraw's art assets.

Outputs (all under assets/):
  - app-icon.png          24x24, 4-color program icon (for png-to-mvicon)
  - default-palette.txt   the 4-color icon palette
  - app-palette.txt       1bpp image palette (idx0=black, idx1=white)
  - sys-images/*.png      21 toolbar button PNGs, 24x24, pure black/white

The toolbar buttons are authored in pure black (0,0,0) and white (255,255,255)
so png-to-os9-image --bits-per-pixel=1 maps them cleanly to palette indices
0 (black) and 1 (white). The patterns drawn here only *represent* the cgfx
PAT_* fills; the real fill is applied at draw time via _cgfx_pset.

Run from the project root:  python3 tools/gen_assets.py
"""
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ASSETS = os.path.join(ROOT, "assets")
SYS = os.path.join(ASSETS, "sys-images")
os.makedirs(SYS, exist_ok=True)

N = 24                      # button / icon size
BLACK = (0, 0, 0)
WHITE = (255, 255, 255)

# CoCo color component 0..3 -> 8-bit sample.
C = (0, 85, 170, 255)
def coco(r, g, b):
    return (C[r], C[g], C[b])


# --------------------------------------------------------------------------- #
# palettes
# --------------------------------------------------------------------------- #
def write_palette(path, entries):
    with open(path, "w") as f:
        for i, (r, g, b) in enumerate(entries):
            f.write("PALET%d=%d,%d,%d\n" % (i, r, g, b))

# 1bpp image palette: only indices 0 (black) and 1 (white) are used at 1bpp.
app_pal = [(0, 0, 0), (3, 3, 3)] + [(0, 0, 0)] * 14
write_palette(os.path.join(ASSETS, "app-palette.txt"), app_pal)

# Icon palette (4 meaningful colors; rest are filler):
#   0 white paper, 1 black ink, 2 dark grey, 3 cyan-blue accent.
icon_pal = [(3, 3, 3), (0, 0, 0), (1, 1, 1), (0, 2, 3)] + [(0, 0, 0)] * 12
write_palette(os.path.join(ASSETS, "default-palette.txt"), icon_pal)

ICON_WHITE = coco(3, 3, 3)
ICON_BLACK = coco(0, 0, 0)
ICON_GREY = coco(1, 1, 1)
ICON_BLUE = coco(0, 2, 3)


# --------------------------------------------------------------------------- #
# helpers
# --------------------------------------------------------------------------- #
def new_btn():
    img = Image.new("RGB", (N, N), WHITE)
    return img, ImageDraw.Draw(img)

def save_btn(img, name):
    img.save(os.path.join(SYS, name))

def border(d):
    d.rectangle([0, 0, N - 1, N - 1], outline=BLACK)


# --------------------------------------------------------------------------- #
# tool buttons
# --------------------------------------------------------------------------- #
def tool_select():
    img, d = new_btn()
    # classic NW-pointing arrow cursor
    pts = [(5, 4), (5, 18), (9, 14), (12, 20), (14, 19), (11, 13), (16, 13)]
    d.polygon(pts, fill=WHITE, outline=BLACK)
    d.polygon(pts, outline=BLACK)
    save_btn(img, "tool_select.png")

def tool_rect():
    img, d = new_btn()
    d.rectangle([4, 6, 19, 17], outline=BLACK)
    save_btn(img, "tool_rect.png")

def tool_frect():
    img, d = new_btn()
    d.rectangle([4, 6, 19, 17], fill=BLACK)
    save_btn(img, "tool_frect.png")

# Type-5 (640x192) pixels are ~2x taller than wide, so a pixel-square circle
# reads as a tall oval. Draw the icon shapes ~2:1 (wide) so the circle looks
# round on screen and the ellipse reads as a flatter wide oval.
def tool_circle():
    img, d = new_btn()
    d.ellipse([2, 6, 21, 17], outline=BLACK)   # 20x12 px -> ~round on screen
    save_btn(img, "tool_circle.png")

def tool_ellipse():
    img, d = new_btn()
    d.ellipse([1, 8, 22, 15], outline=BLACK)   # 22x8 px -> a wide oval on screen
    save_btn(img, "tool_ellipse.png")

def tool_line():
    img, d = new_btn()
    d.line([4, 19, 19, 4], fill=BLACK, width=1)
    save_btn(img, "tool_line.png")


# --------------------------------------------------------------------------- #
# pattern swatches -- the actual cgfx GRP_PAT2 fills.
#
# These are the exact 8x8 tiles from NitrOS-9 sys/stdpats_2.asm (the data the
# GrfDrv loads for the standard 2-color patterns). Each row is one byte, MSB =
# leftmost pixel, a set bit = ink. We tile them full-bleed across the 24x24
# button so the swatch matches what _cgfx_pset(GRP_PAT2, n) actually paints.
# pat0 is solid (the default, not stored in stdpats_2).
# --------------------------------------------------------------------------- #
PAT_TILES = [
    ("pat0.png", [0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]),  # SLD  solid
    ("pat1.png", [0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55]),  # DOT  dots
    ("pat2.png", [0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88]),  # VRT  vertical
    ("pat3.png", [0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x00]),  # HRZ  horizontal
    ("pat4.png", [0xFF, 0x88, 0xFF, 0x88, 0xFF, 0x88, 0xFF, 0x88]),  # XHTC crosshatch
    ("pat5.png", [0x44, 0x88, 0x11, 0x22, 0x44, 0x88, 0x11, 0x22]),  # LSNT left slant
    ("pat6.png", [0x22, 0x11, 0x88, 0x44, 0x22, 0x11, 0x88, 0x44]),  # RSNT right slant
    ("pat7.png", [0x88, 0x00, 0x22, 0x00, 0x88, 0x00, 0x22, 0x00]),  # SDOT small dots
    ("pat8.png", [0x66, 0x99, 0x99, 0x66, 0x66, 0x99, 0x99, 0x66]),  # BDOT large dots
]

# White margin left around each pattern swatch, in pixels.
PAT_MARGIN = 2

def save_pattern(name, rows):
    img, d = new_btn()
    for y in range(PAT_MARGIN, N - PAT_MARGIN):
        bits = rows[y % 8]
        for x in range(PAT_MARGIN, N - PAT_MARGIN):
            if bits & (0x80 >> (x % 8)):
                d.point((x, y), fill=BLACK)
    save_btn(img, name)

def patterns():
    for name, rows in PAT_TILES:
        save_pattern(name, rows)


# --------------------------------------------------------------------------- #
# color swatches
# --------------------------------------------------------------------------- #
def col_black():
    img, d = new_btn()
    d.rectangle([3, 3, 20, 20], fill=BLACK)
    save_btn(img, "col_black.png")

def col_white():
    img, d = new_btn()
    d.rectangle([3, 3, 20, 20], fill=WHITE, outline=BLACK)
    save_btn(img, "col_white.png")


# --------------------------------------------------------------------------- #
# logic-mode glyphs: two overlapping squares A (4..14) and B (9..19),
# with the region(s) filled per boolean op.
# --------------------------------------------------------------------------- #
A = (4, 6, 14, 16)
B = (9, 9, 19, 19)

def in_rect(x, y, r):
    return r[0] <= x <= r[2] and r[1] <= y <= r[3]

def logic_glyph(name, predicate):
    img, d = new_btn()
    for y in range(N):
        for x in range(N):
            a = in_rect(x, y, A)
            b = in_rect(x, y, B)
            if predicate(a, b):
                d.point((x, y), fill=BLACK)
    # outline both squares so the shapes read even where unfilled
    d.rectangle(list(A), outline=BLACK)
    d.rectangle(list(B), outline=BLACK)
    save_btn(img, name)

def logic_none():
    # overlay: source (B) drawn solid over A -> B fully black, A outline only
    logic_glyph("logic_none.png", lambda a, b: b)

def logic_and():
    logic_glyph("logic_and.png", lambda a, b: a and b)

def logic_or():
    logic_glyph("logic_or.png", lambda a, b: a or b)

def logic_xor():
    logic_glyph("logic_xor.png", lambda a, b: a != b)


# --------------------------------------------------------------------------- #
# program icon (24x24, 4-color): white canvas with a black rectangle and a
# blue circle overlapping, plus a grey shadow -- a "shapes" motif.
# --------------------------------------------------------------------------- #
def app_icon():
    img = Image.new("RGB", (N, N), ICON_WHITE)
    d = ImageDraw.Draw(img)
    d.rectangle([0, 0, N - 1, N - 1], outline=ICON_BLACK)      # frame
    d.rectangle([4, 5, 14, 15], outline=ICON_GREY)             # soft back square
    d.ellipse([8, 9, 20, 21], fill=ICON_BLUE, outline=ICON_BLACK)  # blue circle
    d.rectangle([3, 4, 13, 14], outline=ICON_BLACK)            # black square on top
    d.line([3, 20, 9, 14], fill=ICON_BLACK)                    # a little "pen" stroke
    img.save(os.path.join(ASSETS, "app-icon.png"))


def main():
    tool_select(); tool_rect(); tool_frect(); tool_circle(); tool_ellipse(); tool_line()
    patterns()
    col_black(); col_white()
    logic_none(); logic_and(); logic_or(); logic_xor()
    app_icon()
    # sanity: report what we wrote
    pngs = sorted(os.listdir(SYS))
    print("sys-images (%d):" % len(pngs), ", ".join(pngs))
    for p in pngs + ["../app-icon.png"]:
        im = Image.open(os.path.join(SYS, p))
        assert im.size == (N, N), "%s is %s" % (p, im.size)
    print("app-icon.png:", Image.open(os.path.join(ASSETS, "app-icon.png")).size)
    print("OK")


if __name__ == "__main__":
    main()
