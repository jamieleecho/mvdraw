# CLAUDE.md — CoCo3 / NitrOS-9 / Multi-Vue / MVKit / cgfx notes

Hard-won, non-obvious lessons from building this app. Most of it transfers to
any Multi-Vue/cgfx program built with the cmoc toolchain.

## Toolchain (cmoc + coco-dev Docker)

- Builds run inside the `jamieleecho/coco-dev` image. The vendored `./coco-dev`
  wrapper uses `docker run -it`, which **fails in a non-TTY/agent shell**. Invoke
  docker directly without `-t`:
  ```
  docker run --rm -v /Users:/home -e HOME=/home/<user> \
    -w /home/<user>/src/<proj> jamieleecho/coco-dev:<ver> make
  ```
- **Every `docker run` is a fresh container.** Anything the build installs into
  the image (e.g. MVKit headers/lib into `/usr/local/share/cmoc`) does **not**
  persist between runs — the bootstrap (clone `cmoc_os9`, `make -C mvkit install`)
  must run on every build. If you see `mvkit/mv_defs.h: No such file or directory`,
  the install step didn't run.
- **cmoc language limits:** 16-bit `int`; 32-bit `long` **is** available; **no
  floating point**; **no `<stddef.h>`** (use `0`/`(char *)0`, not `NULL`); strict
  `?:` pointer-vs-int typing (cast the null branch: `(argc==2)?argv[1]:(char*)0`).
- `MV_OUTPATH` and `Flush()` live in `<mvkit/mv_defs.h>` — a file that draws via
  cgfx but doesn't include `mvkit.h` must include it explicitly.
- Verify the build produced a valid module: `os9 ident <disk>,CMDS/<app>` →
  CRC should read **Good**. A non-clean disk copy (incremental rebuild over a disk
  that already has the files) can error with "pathname not found" / permission —
  `make clean` first.

## cgfx / Multi-Vue display model

- **Screen type 5 = 640×192 px = 80×24 character cells (8×8), 1 bpp** (two colors,
  palette registers 0 and 1). `_cgfx_setgc`/`settype` map stype 5/7 → 80 cols.
- **Pixel aspect is ~2:1** — vertical pixels are ~2× taller than horizontal pixels
  are wide. Consequences:
  - A pixel-square circle (or icon) renders as a **tall oval**. Draw shapes/icons
    ~2:1 *wide* to look round.
  - For a true on-screen circle from a center+radius, fold the aspect into the
    radius: `r = isqrt(dx² + (2·dy)²)`, giving horizontal radius `r`, vertical `r/2`.
- `bar`/`box`/`rbar`/`circle` **fill/draw with the foreground color**
  (`_cgfx_fcolor`), not the background. (Confirmed by cgfx's own button widget,
  which does `fcolor(bg); rbar` to paint a button background.)
- **Fill patterns** (`_cgfx_pset(path, GRP_PAT2, PAT_*)`) require the 2-color
  pattern buffers to be **loaded first**; selecting an unpopulated group paints
  nothing. The standard buffers ship as NitrOS-9 `SYS/stdpats_2` (gpload format);
  the canonical 8×8 tiles are in `nitros9/level2/sys/stdpats_2.asm` (one byte per
  row, MSB = leftmost, set bit = ink).
- **`_cgfx_cwarea(path, cpx, cpy, szx, szy)` is the gotcha-heavy one:**
  - Coordinates are in **character cells**, not pixels. So a clipped draw region
    must be **char-aligned** (origin and size multiples of 8 px).
  - It **clips AND shifts the origin** to `(cpx,cpy)` — drawing inside becomes
    relative to that corner.
  - **It also re-bases the mouse:** coordinates from `_cgfx_gs_mouse` / click
    events are relative to the *currently active* working area. So set the canvas
    working area **only around drawing**, never while reading the mouse — otherwise
    the initial click (read under the standard area) and in-drag reads (under the
    canvas area) use different origins and disagree.
  - The window's content area excludes a one-cell border; `view.x/view.y` from the
    framework are relative to that border origin, so a canvas clip computed as
    `view.x/8` is off by the border (`WINDOW_CCOL/CROW`) cells unless you add it.
- `_cgfx_scalesw(path, false)` **once at startup** so 1 unit == 1 pixel (the
  framework already does this, but cwarea cell math depends on it).
- A primitive that extends **beyond** the active working area may be **rejected
  entirely** (draws nothing → black), not clipped. A "too big" fill vanishing is a
  sign the geometry left the working area.

## MVKit patterns

- `mv_app_run` owns the loop; lifecycle is `pre_init`/`init`/`menu_actions`/
  `refresh_menus_action`/`application_action`. Menu enabled-state is set in
  `refresh_menus_action` and only re-read on `mv_app_refresh_menubar()` — call it
  after any event (e.g. a canvas click that changes selection) that should
  enable/disable a menu item.
- `MVImageGrid` single-select: `mv_image_grid_init` sets `selected=0` but does
  **not** fire the item callback; only `mv_image_grid_select` does. So default
  tool/colour/etc. state must be set explicitly, or initialized to match index 0.
- `MVDocument` already depends on the message-box dialog, so save-prompt logic
  (e.g. `mv_document_close` → "Save changes?") belongs in that layer, reusable
  across apps, not duplicated in each app's close handler.
- Drag idiom: read button-down from the event, then loop
  `do { _cgfx_gs_mouse(MV_OUTPATH,&mp); … } while (mp.pt_cbsa);` reading
  `pt_wrx`/`pt_wry`/`pt_cbsa`. Draw drag/resize previews with **XOR** (draw twice
  to erase) for speed; full-repaint only on release.

## Working method on this platform

- **You cannot render the GUI headlessly.** Coordinate-system and API-semantics
  assumptions — *does this call shift the origin? cells or pixels? size = count or
  delta?* — must be confirmed by a real run. State the assumption explicitly, build,
  and have the user verify; don't flip-flop between guesses. Symptoms map cleanly to
  causes (e.g. "first point off by a constant + drag point off by a different
  amount" = mouse read under two different working-area origins).
- **Integer-only geometry:** point-to-segment distance needs 32-bit intermediates
  (a cross product overflows 16-bit). Avoid the squared-distance compare
  (`cross²` overflows even 32-bit) — take an integer `sqrt` of the length and
  compare `|cross| ≤ tol·len`.
