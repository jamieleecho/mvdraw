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

def tool_circle():
    img, d = new_btn()
    d.ellipse([4, 4, 19, 19], outline=BLACK)
    save_btn(img, "tool_circle.png")

def tool_ellipse():
    img, d = new_btn()
    d.ellipse([3, 7, 20, 16], outline=BLACK)
    save_btn(img, "tool_ellipse.png")

def tool_line():
    img, d = new_btn()
    d.line([4, 19, 19, 4], fill=BLACK, width=1)
    save_btn(img, "tool_line.png")


# --------------------------------------------------------------------------- #
# pattern swatches (representations of cgfx PAT_*)
# --------------------------------------------------------------------------- #
PAT_BOX = (3, 3, 20, 20)   # inclusive pixel area to fill with the pattern

def fill_pattern(d, fn):
    x0, y0, x1, y1 = PAT_BOX
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if fn(x - x0, y - y0):
                d.point((x, y), fill=BLACK)

def pat_solid():
    img, d = new_btn()
    d.rectangle(PAT_BOX, fill=BLACK)
    border(d)
    save_btn(img, "pat0.png")

def pat_dot():
    img, d = new_btn()
    fill_pattern(d, lambda x, y: (x % 3 == 0) and (y % 3 == 0))
    border(d); save_btn(img, "pat1.png")

def pat_vrt():
    img, d = new_btn()
    fill_pattern(d, lambda x, y: x % 3 == 0)
    border(d); save_btn(img, "pat2.png")

def pat_hrz():
    img, d = new_btn()
    fill_pattern(d, lambda x, y: y % 3 == 0)
    border(d); save_btn(img, "pat3.png")

def pat_xhtc():
    img, d = new_btn()
    fill_pattern(d, lambda x, y: (x % 4 == 0) or (y % 4 == 0))
    border(d); save_btn(img, "pat4.png")

def pat_lsnt():
    img, d = new_btn()
    fill_pattern(d, lambda x, y: (x + y) % 4 == 0)
    border(d); save_btn(img, "pat5.png")

def pat_rsnt():
    img, d = new_btn()
    fill_pattern(d, lambda x, y: (x - y) % 4 == 0)
    border(d); save_btn(img, "pat6.png")

def pat_sdot():
    img, d = new_btn()
    fill_pattern(d, lambda x, y: (x % 4 == 1) and (y % 4 == 1))
    border(d); save_btn(img, "pat7.png")

def pat_bdot():
    img, d = new_btn()
    # 2x2 dots on a 5-pixel grid
    fill_pattern(d, lambda x, y: (x % 5 in (0, 1)) and (y % 5 in (0, 1)))
    border(d); save_btn(img, "pat8.png")


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
    pat_solid(); pat_dot(); pat_vrt(); pat_hrz(); pat_xhtc()
    pat_lsnt(); pat_rsnt(); pat_sdot(); pat_bdot()
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
