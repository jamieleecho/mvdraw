# mvdraw

A mouse-driven **vector drawing program** for the Tandy Color Computer 3 under
Multi-Vue (NitrOS-9 Level 2), built with [MVKit](mvkit/). It draws rectangles,
filled rectangles, circles, ellipses, and lines on a black-and-white canvas
(screen type 5, 640×192), with image-grid toolbars for the shape tool, fill
pattern, color, and logic-drawing mode.

## Features

- **Shape tools** (toolbar, top-left): Select, Rectangle, Filled-Rect, Circle,
  Ellipse, Line. Keys `1`–`6` also pick a tool.
- **Patterns** (toolbar, top): the nine cgfx fill patterns.
- **Color**: black / white (the only two colors at 1 bpp).
- **Logic mode**: None (overlay), And, Or, Xor.
- **Draw** by point-click-drag. **Select** mode picks a shape (corner handles),
  drag a handle to **resize** or the body to **move**. During a drag the shape
  is previewed with fast **XOR rubber-banding**; the canvas fully repaints only
  on mouse-release (the CoCo is slow).
- **File**: New / Open… / Save / Save As… (documents use the `.mvd` extension).
- **Undo** (Edit ▸ Undo, or `Ctrl-Z`) backed by MVKit's document/undo manager.

## Building

All builds run inside the `jamieleecho/coco-dev` Docker toolchain image. The
top-level `Makefile` bootstraps everything (clones + builds `cmoc_os9`, installs
the vendored MVKit) on the first run.

```sh
./coco-dev make            # -> build/mvdraw.os9   (first run is slow)
./coco-dev make clean
```

`./coco-dev` opens an interactive shell; for non-interactive/CI use run docker
directly (no TTY):

```sh
docker run --rm -v /Users:/home -e HOME=/home/$USER \
  -w "/home/$(pwd | sed 's#^/Users/##')" jamieleecho/coco-dev:latest make
```

## Running

```sh
make run                   # launches build/mvdraw.os9 in MAME (host, needs a display)
```

In the CoCo 3 window: after Multi-Vue loads, open the `/dd` drive and double-click
the **mvdraw** icon.

## Regenerating art

The icon and toolbar button PNGs are generated:

```sh
python3 tools/gen_assets.py     # writes assets/app-icon.png + assets/sys-images/*.png
```

## Layout

| Path | What |
|------|------|
| `mvdraw.c` | app: theme, menus, four toolbars, document + event wiring |
| `drawing.c` / `.h` | the document model (shapes) + save/load + undo helpers |
| `draw_view.c` / `.h` | the editable canvas view (draw, XOR preview, drag/resize/move) |
| `tools/gen_assets.py` | PIL generator for the icon + toolbar PNGs |
| `mvkit/` | the MVKit framework (vendored) |
| `disks/` | the NitrOS-9 base disk image |
