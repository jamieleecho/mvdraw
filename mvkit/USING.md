# Using MVKit

A step-by-step guide to building Multi-Vue applications for the Tandy Color
Computer 3 with MVKit.

> This guide is built up in stages. Each stage has a complete, runnable example
> under [`guide/`](guide) that you can build, run, and diff
> against the next. This first part covers the *why*, the requirements, and
> installation; the app-building stages follow.

## Introduction

MVKit is an **opinionated application framework** for writing windowed programs
that run under [Multi-Vue](https://en.wikipedia.org/wiki/Multi-Vue) on NitrOS-9
— think of it as sitting down with AppKit, but for a 1986 8-bit machine.

Multi-Vue gives the CoCo 3 a mouse-driven, windowed desktop with menus,
dialogs, and a window manager. Writing for it directly means a lot of low-level
window-manager bookkeeping. MVKit's job is to take that bookkeeping off your
hands so you can write a real app — menus and actions, a document with
open/save/undo, image views, modal dialogs — in plain C.

### Philosophy

- **Opinionated, so you don't have to decide.** MVKit picks one good way to do
  the common things: a run loop with lifecycle callbacks
  (`mv_app_run()`), `NSDocument`-style file handling
  (`mv_document.h`), a minimal view protocol (`mv_view.h`)
  with a ready-made image-button grid (`mv_image_grid.h`),
  built-in modal dialogs, and a palette/theme model
  (`mv_theme.h`) that keeps the system chrome looking right.

- **Modern tools, retro target.** You write C (compiled with
  [cmoc](http://sarrazip.com/dev/cmoc.html)) and draw art as PNGs; the toolchain
  converts PNGs to OS-9 images, assembles an OS-9 disk, and runs it in
  [MAME](https://www.mamedev.org/). The whole toolchain lives in a Docker image,
  so your machine stays clean and builds are reproducible.

- **Easy by default, correct by construction.** Sensible defaults that look
  right (the window-chrome color ramp), guard rails that fail at *compile* time
  rather than on the CoCo (e.g. menu-window size floors), and a small RAM
  footprint (one public function per translation unit, so the linker pulls in
  only what you use).

- **Self-contained and installable.** Install MVKit once into cmoc's shared
  directory and every app just does `#include <mvkit/mvkit.h>` and links
  `-lmvkit` — no `-I`/`-L` juggling.

If you have used AppKit or any retained-mode UI framework, MVKit will feel
familiar; if you have written CoCo assembly, it will feel like a holiday.

## Requirements

This is the part that takes the most setup. You need:

- **Docker** (Desktop 4.x / Engine 24+). The entire CoCo toolchain —
  [cmoc](http://sarrazip.com/dev/cmoc.html) (C compiler),
  [LWTOOLS](https://www.lwtools.ca) (`lwasm`/`lwar`/`lwlink` assembler & linker),
  [ToolShed](https://github.com/nitros9project/toolshed) (`os9` disk tools),
  the PNG→OS-9 image converters, and Doxygen — ships in the
  [`jamieleecho/coco-dev`](https://github.com/jamieleecho/coco-dev) image
  (`>= 0.83`). You do **not** install any of these on your host. All build
  commands below run *inside* that image (via a dev container, CI, or
  `docker run`).

- **[cmoc_os9](https://github.com/nitros9project/cmoc_os9)** — the OS-9 C
  library (`libc`) and CoCo graphics library (`libcgfx`) that MVKit and your app
  link against. It is cloned and built for you by the project Makefile (pinned to
  a known-good commit), so you do not fetch it by hand.

- **A NitrOS-9 Level 2 CoCo 3 base disk image.** Your app is copied onto a
  bootable NitrOS-9 disk; the build starts from a base image (this repo ships
  one under `disks/`). This is the master disk Multi-Vue boots from.

- **[MAME](https://www.mamedev.org/) 0.269+** with the `coco3` driver, plus the
  CoCo 3 ROMs, **only if you want to run your app on an emulator** (you do). MAME
  runs on the *host* (it needs a display), not in the Docker image. Real CoCo 3
  hardware (512 KB+, an 80-track drive, a mouse) works too.

- **`make` and `git`** on the host, to drive the build and fetch dependencies.
  Everything else (the compiler, image converters, disk tools) runs inside the
  `coco-dev` image, so you need no host Python toolchain.

The good news: once Docker is installed and you can run the `coco-dev` image,
everything else is fetched and built by `make`.

## Installation

MVKit installs into cmoc's shared directory (`/usr/local/share/cmoc`), which
cmoc searches automatically — so after installing, an app needs no `-I`/`-L` for
MVKit. From inside the `coco-dev` environment:

```sh
# in the mvkit/ directory
make            # build libmvkit.a
make install    # copy headers + libmvkit.a into cmoc's shared dir
```

`make install` places the headers under
`/usr/local/share/cmoc/include/mvkit/`, the `<stdbool.h>` shim (cmoc ships
none) at the include root, and `libmvkit.a` in
`/usr/local/share/cmoc/lib/`. Override `PREFIX` (and set `DESTDIR`) for a staged
or packaged install; `make uninstall` reverses it. See the
[MVKit README](README.md) for the build details.

With MVKit installed, the smallest possible program is:

```c
#include <mvkit/mvkit.h>
```

and it links with `-lmvkit -lc -lcgfx`. The next stages turn that into a real
windowed app — and introduce `app.mk`, the small reusable Makefile that reduces
each app's build to a few lines.

## Building the examples

Each stage's app lives under `mvkit/guide/`. From the repo root, inside
the coco-dev environment:

```sh
make                 # one-time: fetch + build cmoc_os9 and install MVKit
make mvkit-guide     # build every mvkit/guide/NN-* app
```

The first `make` clones/builds cmoc_os9 and installs MVKit (what the examples
link against); `make mvkit-guide` then builds each example into its own
`build/<name>.os9`. To build or run a single example:

```sh
make -C mvkit/guide/01-hello          # -> build/hello.os9
make -C mvkit/guide/01-hello run      # launch it in MAME (on the host)
```

(MAME runs on the host, not in the container, since it needs a display.)

## A simple app

The smallest real MVKit app opens a window and runs. The complete example is in
[`guide/01-hello`](https://github.com/jamieleecho/xmastree/tree/main/mvkit/guide/01-hello).

<img src="https://raw.githubusercontent.com/jamieleecho/xmastree/main/mvkit/guide/01-hello/screenshot.png" alt="01-hello — a framed window printing Loaded." width="512">

`main.c`:

```c
#include <stdio.h>
#include <unistd.h>          /* sleep */

#include <mvkit/mvkit.h>

mv_menu_none(hello_window, "hello");

static void hello_init(void) {
    printf("Loading...\n");
    Flush();
    sleep(3);
    printf("Loaded.\n");
    Flush();
}

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &hello_window,
        mv_app_pre_init_nop, hello_init, mv_app_menu_actions_nop,
        mv_app_refresh_menus_action_nop, mv_app_event_nop);
}
```

What's going on:

- **`mv_menu_none(hello_window, "hello")`** declares the window descriptor: a
  framed window titled *hello* with no menu bar. (The next stage adds menus.)
- **`mv_app_run(...)`** opens the window and runs the event loop until the user
  closes the window — the close box quits by default. It takes the process
  arguments, the window, and five callbacks; here all but `init` are the
  framework's built-in no-ops (`mv_app_*_nop`), so you fill in only what you use.
- **`init`** runs once, just after the window is set up. Text written to stdout
  lands in the window, so this prints `Loading...`, sleeps three seconds, then
  prints `Loaded.` — showing that `init` finishes before the event loop begins.
  `Flush()` pushes buffered drawing to the screen. The earlier **`pre_init`**
  callback (a no-op here) runs *before* the window exists; it's where palette and
  resource setup belong (later stages).

The Makefile is three lines:

```make
APP  := hello
SRCS := main.c
include ../guide.mk
```

`guide.mk` points `app.mk` at this repo's vendored cmoc_os9 and base disk. A
standalone app outside this repo would instead just
`include /usr/local/share/cmoc/app.mk` (installed by `make install`), set
`BASEIMAGE` to its NitrOS-9 disk, and go. From those two variables `app.mk`
compiles and links the program, builds the icon, generates the Multi-Vue
launcher AIF, and assembles a bootable disk image.

Build it inside the coco-dev image; run it with MAME on the host:

```sh
make            # -> build/hello.os9
make run        # launch it in MAME
```

Launched from the Multi-Vue desktop, the window prints `Loading...`, pauses,
then `Loaded.`.

## A menu and an action

Stage 1's window had no menu bar. This stage adds a **Help** menu with an
**About…** item that opens a message box. The example is in
[`guide/02-menu`](https://github.com/jamieleecho/xmastree/tree/main/mvkit/guide/02-menu).

<img src="https://raw.githubusercontent.com/jamieleecho/xmastree/main/mvkit/guide/02-menu/screenshot.png" alt="02-menu — choosing Help ▸ About… opens a message box" width="512">

`main.c`:

```c
#include <stdio.h>

#include <mvkit/mvkit.h>

#define MN_HELP 30   /* an app-chosen menu id (cgfx reserves the low numbers) */

static MIDSCR help_items[] = {
    MV_MENU_ITEM("About..."),
};

static MNDSCR menus[] = {
    MV_MENU("Help", MN_HELP, help_items),
};

mv_set_menus(menu_window, "menu", menus);

static void menu_init(void) {
    printf("Choose Help > About...\n");
    Flush();
}

static void about_action(MSRET *msinfo, int menuid, int itemno) {
    mv_app_show_message_box("menu example\r\nMVKit guide, stage 2",
                            MVMessageBoxType_Info);
}

static const MVMenuItemAction menu_actions[] = {
    {MN_HELP, 1, about_action},   /* Help item 1 ("About...") */
    MV_MENU_ACTION_END            /* catch-all: ignore everything else */
};

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &menu_window,
        mv_app_pre_init_nop, menu_init, menu_actions,
        mv_app_refresh_menus_action_nop, mv_app_event_nop);
}
```

Declaring the menu:

- A menu is a `MIDSCR items[]` table wrapped in a `MNDSCR menus[]` table.
  `MV_MENU_ITEM("About...")` is one item; `MV_MENU("Help", MN_HELP, help_items)`
  is the Help menu with the app-chosen id `MN_HELP`. `mv_set_menus(window, ...)`
  replaces stage 1's `mv_menu_none` with a window that carries `menus`. These
  macros hide the cgfx structs' reserved fields (see `mv_menu.h`).

Handling the selection:

- `menu_actions[]` is a dispatch table of `MVMenuItemAction` rows. When the user
  picks Help ▸ About… (menu `MN_HELP`, item `1`), `mv_app_run` calls
  `about_action`; the trailing catch-all row matches anything without its own
  entry. `MV_MENU_ACTION_END` is that row — a sentinel using the framework's
  built-in no-op handler, so you write no stub of your own. (Put a real
  `{-1, -1, handler}` there instead if you want to act on unhandled items.)
- `about_action` opens a modal box with `mv_app_show_message_box(message, type)`.
  `MVMessageBoxType_Info` shows a lone **OK**; embed `\r\n` to wrap the message
  onto more lines. (The full set of dialogs comes in a later stage.)

Build and run as before:

```sh
make -C mvkit/guide/02-menu        # -> build/menu.os9
make -C mvkit/guide/02-menu run
```

The window opens with a **Help** menu; choosing **About…** pops the message box,
and the window close box quits the app.

## Window types, colors, and palettes

This stage installs a color palette and draws an image that was converted from a
PNG at build time. The example is in [`guide/03-palette`](https://github.com/jamieleecho/xmastree/tree/main/mvkit/guide/03-palette).

<img src="https://raw.githubusercontent.com/jamieleecho/xmastree/main/mvkit/guide/03-palette/screenshot.png" alt="03-palette — an image converted from a PNG, drawn in the window" width="512">

### Window types

`mv_app_run(...)` opens a plain **framed** window (cgfx `WT_FWIN`). For one with
scrollbars, swap in `mv_app_run_with_scrollbars(...)` (`WT_FSWIN`) — same
arguments. Both are thin macros over `mv_app_run_typed`, which takes the window
type directly if you need another.

### Colors and the palette

The CoCo 3 draws from a 16-entry palette. MVKit wraps it as an `MVTheme` and
installs it in `pre_init` with `mv_app_set_theme`:

```c
static const MVTheme theme = { {
    0x00, 0x07, 0x38, 0x3f,   /* 0-3  chrome ramp: black, dark grey, light grey, white */
    0x24, 0x12, 0x09, 0x36,   /* 4-7  red, green, blue, yellow */
    0x1b, 0x2d, 0x20, 0x10,
    0x08, 0x04, 0x07, 0x38
} };

mv_app_set_theme(&theme);
```

The values are cgfx color numbers. **Registers 0-3 are the window-chrome ramp**
and must run darkest → lightest: the OS-9 window manager draws the menu bar,
dialog borders, shadows, and scrollbars from those four, so a wrong order makes
the chrome look broken. Registers 4-15 are yours. (MVKit ships
`mv_theme_default` with the standard ramp if you don't need a custom one — see
`mv_theme.h`.) Code names registers by index, e.g. `_cgfx_bcolor(MV_OUTPATH, 0)`
clears the window to chrome-darkest.

### Images from PNGs

Art is authored as a PNG, converted to an OS-9 image at build time, and loaded
at run time:

1. Drop a PNG in `assets/sys-images/`. `app.mk` runs `png-to-os9-image` on each,
   producing a palette-indexed `.i09` under `SYS/<app>/` on the disk.
2. The conversion maps each pixel to the nearest entry in
   **`assets/app-palette.txt`** — which lists the *same 16 colors* as your
   `MVTheme`, written as CoCo `R,G,B` (0-3). **The two must agree:** a pixel that
   is red in the PNG becomes index 4, and the installed theme draws index 4 as
   red. (They're two encodings of one palette — keep them in sync.)
3. Load and draw it:

```c
mv_image_init("palette");                 /* resources in /dd/SYS/palette/ */
mv_image_load_resource("logo.i09", 2);    /* into image buffer 2 */
...
mv_image_draw(2, 8, 8);                    /* blit at (8, 8) */
```

`make -C mvkit/guide/03-palette run` opens the window with the converted image
drawn on a black background.

## The image grid

`MVImageGrid` is a grid of single-select image buttons — MVKit's first **view**
(it embeds an `MVView`). The example is in [`guide/04-grid`](https://github.com/jamieleecho/xmastree/tree/main/mvkit/guide/04-grid): four
buttons in a 2-column grid.

<img src="https://raw.githubusercontent.com/jamieleecho/xmastree/main/mvkit/guide/04-grid/screenshot.png" alt="04-grid — a 2×2 grid of shape buttons, the selected one outlined" width="512">

```c
#define NUM_BUTTONS 4
#define COLUMNS     2

static MVImageGrid grid;
static int image_ids[NUM_BUTTONS] = { 2, 3, 4, 5 };   /* image buffer per button */

static void grid_selected(MVImageGrid *g) {
    printf("Selected button %d\n", mv_image_grid_selected(g));
    Flush();
}

/* pre_init: load one image (stage 3) into each buffer */
mv_image_load_resource("btn1.i09", image_ids[0]);   /* ... btn2..btn4 */

/* init: lay out and draw the grid */
mv_image_grid_init(&grid, 8, 8, NUM_BUTTONS, COLUMNS, image_ids, grid_selected);

/* application_action: route content-area clicks to the grid */
static void grid_action(MVUiEvent *event) {
    if (event->event_type == MVUiEventType_MouseClick) {
        mv_view_dispatch_click(&grid.view, event);
    }
}
```

How it fits together:

- **Images** — `image_ids[i]` is the image buffer holding button *i*'s picture
  (loaded with `mv_image_load_resource`). The grid blits each into its cell.
- **Layout** — `mv_image_grid_init(grid, x, y, num_items, columns, image_ids,
  on_select)` arranges the buttons (default 24×24 cells) and draws them
  immediately; the selected item is shown with an outline box (the image keeps
  its true colors).
- **Clicks** — the grid is an `MVView`, so an app routes content clicks to it:
  `mv_view_dispatch_click(&grid.view, event)` hit-tests, updates the selection,
  and fires `on_select`. (There's no automatic view hierarchy yet — you keep
  your views and dispatch to them; that's what `application_action` is for.)
- **Selection** — `mv_image_grid_selected(grid)` reads the current index;
  `mv_image_grid_select(grid, i)` sets it programmatically (e.g. a key shortcut).

`make -C mvkit/guide/04-grid run`, then click a button — its index prints and
the highlight moves.

## Dialogs

MVKit's modal dialogs: a message box and the file open/save browsers. The
example is in [`guide/05-dialogs`](https://github.com/jamieleecho/xmastree/tree/main/mvkit/guide/05-dialogs) — a "Dialogs" menu whose
items open each one and report the result.

<img src="https://raw.githubusercontent.com/jamieleecho/xmastree/main/mvkit/guide/05-dialogs/screenshot.png" alt="05-dialogs — the file Open browser, with hello.txt listed" width="512">

### Message box

```c
MVMessageBoxResult r = mv_app_show_message_box("Proceed?", MVMessageBoxType_YesNo);
/* r == MVMessageBoxResult_Yes or _No */
```

`mv_app_show_message_box(message, type)` shows a modal box and returns the button
pressed. `type` selects the buttons — `Info` / `Warning` / `Error` (a lone OK),
`OkCancel`, or `YesNo`. The result is `Ok`/`Yes` (the primary button, value 0) or
`Cancel`/`No`. Embed `\r\n` in `message` to wrap onto up to three lines.

### File open / save

```c
char path[MV_PATH_MAX];
if (mv_app_show_open_dialog(path, ".txt")) {   /* or NULL to list every file */
    /* path holds the chosen file */
}
```

`mv_app_show_open_dialog(path, ext)` shows a file browser centered on screen,
copies the chosen name into `path` (a buffer of at least `MV_PATH_MAX` bytes),
and returns it — or `NULL` if cancelled. `ext` (with the dot, e.g. `".txt"`, or
`NULL` for all) filters the listing. `mv_app_show_save_dialog` is the same but
adds a "[new file]" entry and appends `ext` to a typed name that lacks it.

The example ships a `hello.txt` at the disk root (via `app.mk`'s `DATA_FILES`,
which copies extra files onto the disk), so the Open browser has a `.txt` to
list. `make -C mvkit/guide/05-dialogs run`, then pick from the Dialogs menu;
each choice prints its result to the window.

## Documents

`mv_document` wraps your data model with the standard file lifecycle — New /
Open / Save / Save As / Revert — plus dirty tracking and the prompts that guard
against throwing away unsaved work (loosely after `NSDocument`). The example is
in [`guide/06-document`](https://github.com/jamieleecho/xmastree/tree/main/mvkit/guide/06-document): the model is a single integer
counter, and a **Count ▸ Increment** item changes it.

<img src="https://raw.githubusercontent.com/jamieleecho/xmastree/main/mvkit/guide/06-document/screenshot.png" alt="06-document — the counter document showing its count and modified state" width="512">

### Your model and three callbacks

A document is your model plus three callbacks that act on it for a path; each
returns 0 on success or an error code, and any may be `NULL` to disable that
operation:

```c
typedef struct { int count; } Counter;
static Counter counter;
static MVDocument doc;

static int counter_new (void *m, const char *path) { ((Counter *)m)->count = 0; return 0; }
static int counter_open(void *m, const char *path) { /* read the file into *m  */ }
static int counter_save(void *m, const char *path) { /* write *m to the file   */ }

mv_document_init(&doc, NULL, "counter", ".cnt", &counter,
                 counter_new, counter_open, counter_save);
```

`mv_document_init(doc, path, default_path, extension, model, new, open, save)`
ties it together. Pass a `path` to open a file immediately; pass `NULL` to start
a new, unsaved document at `default_path`. (`open`/`save` use the same low-level
`open`/`read`/`write`/`creat`/`close` calls any OS-9 C program does — see the
example.)

### Driving it from the menu

The File-menu items map straight onto the API; each returns whether the model
changed so you can redraw:

| Menu item   | Call                       | What it does |
|-------------|----------------------------|--------------|
| New         | `mv_document_new(&doc)`    | empty model (prompts to save if dirty) |
| Open…       | `mv_document_open(&doc)`   | Open dialog, then load |
| Save        | `mv_document_save(&doc)`   | save to the current path (or Save As if untitled) |
| Save As…    | `mv_document_save_as(&doc)`| Save dialog, then write |
| Revert      | `mv_document_revert(&doc)` | reload from disk, discarding changes |

For a document named on the command line, load it yourself and call
`mv_document_opened(&doc)` to mark it file-backed:

```c
if (argc == 2) { counter_open(&counter, argv[1]); mv_document_opened(&doc); }
```

### Dirty tracking and changes

Record every model change with `mv_document_make_change` — it marks the document
dirty and remembers how to undo the change:

```c
counter.count += 1;
MVUndoItem undo = { counter_undo_increment, &counter };
mv_document_make_change(&doc, &undo);
```

`mv_document_is_dirty(&doc)` then reports unsaved work; New/Open/Revert/close use
it to prompt before discarding. The `mv_document_can_*` queries say which
operations are currently available — drive the menu's enabled state from them in
your `refresh_menus` callback:

```c
mv_menu_item_set_enabled(file_items, FileIndex_Save,   mv_document_is_dirty(&doc));
mv_menu_item_set_enabled(file_items, FileIndex_Revert, mv_document_can_revert(&doc));
```

`make -C mvkit/guide/06-document run`: Increment to dirty it, Save to write
`counter` to disk, Open it back, Revert to undo unsaved changes. The undo item
recorded above isn't used yet — the next stage adds an Undo menu that does.

## Undo

Stage 6 already recorded *how to reverse* every change — the `MVUndoItem` it
passed to `mv_document_make_change`. This stage spends that. The example,
[`guide/07-undo`](https://github.com/jamieleecho/xmastree/tree/main/mvkit/guide/07-undo), is the counter document plus an **Edit ▸ Undo**
menu; the only additions are that menu, an `undo_action`, and one line in
`refresh_menus`.

<img src="https://raw.githubusercontent.com/jamieleecho/xmastree/main/mvkit/guide/07-undo/screenshot.png" alt="07-undo — the counter document with an Edit ▸ Undo menu" width="512">

An undo item is a function plus an argument — "to reverse this change, call
`undo_function(object)`":

```c
static void counter_undo_increment(void *object) { ((Counter *)object)->count -= 1; }
...
MVUndoItem undo = { counter_undo_increment, &counter };
counter.count += 1;
mv_document_make_change(&doc, &undo);   /* do the change; record its inverse */
```

`mv_document_undo(&doc)` pops the most recent change and runs its
`undo_function`, returning whether anything was undone:

```c
static void undo_action(MSRET *msinfo, int menuid, int itemno) {
    if (mv_document_undo(&doc)) {
        show();
    }
}
```

The document's undo manager is bounded (the oldest change is dropped past
`MV_UNDO_MANAGER_MAX_UNDOS`) and it *is* the dirty-tracker: it counts changes
since the last save, so undoing back to the saved point clears "modified" again.
Enable the menu item only when there's something to undo:

```c
mv_menu_item_set_enabled(edit_items, EditIndex_Undo, mv_document_can_undo(&doc));
```

`make -C mvkit/guide/07-undo run`: Increment a few times, then Undo to walk the
count back — watch the `[modified]`/`[saved]` marker flip as you cross the last
save point.

## Where to go next

That's the tour: a windowed app, menus and actions, themes and palettes, the
image grid, dialogs, a document, and undo — each example building on the last.
From here, the [API reference](https://jamieleecho.github.io/xmastree/) documents
every public function and macro, and the [`xmastree`](https://github.com/jamieleecho/xmastree/tree/main/xmastree) app shows the
whole framework working together in something larger than a guide example. Copy
the example whose shape is closest to what you're building and grow it.
