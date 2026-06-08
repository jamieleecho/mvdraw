# MVKit

An opinionated framework for building **Multi-Vue** applications on the Tandy
Color Computer 3 under NitrOS-9, loosely inspired by Apple's AppKit — for a much
lowlier 8-bit machine.

MVKit was extracted from the [xmastree](../xmastree) example app, which remains
its living integration test. The long-term home for this code is a dedicated
section of [cmoc_os9](https://github.com/nitros9project/cmoc_os9); the directory
layout here is chosen to make that move a near-verbatim copy.

## Layout

```
include/mvkit/   public headers; apps include <mvkit/mvkit.h>
src/             implementation
Makefile         builds libmvkit.a
```

## Building

The cmoc / OS-9 toolchain lives in the `jamieleecho/coco-dev` Docker image, not
on the host. From this directory inside the container:

```
make            # builds libmvkit.a
make install    # installs the headers + libmvkit.a into PREFIX (default:
                # /usr/local/share/cmoc, cmoc's shared dir)
```

cmoc auto-searches `PREFIX/include` and `PREFIX/lib`, so once installed an app
just does:

```c
#include <mvkit/mvkit.h>
```

and links with `-lmvkit -lc -lcgfx` — no `-I`/`-L` needed for MVKit. (Override
`PREFIX`, and set `DESTDIR`, for staged or packaged installs; `make uninstall`
reverses it.) To build against the source tree without installing, pass
`-Imvkit/include -Lmvkit` instead.

## API documentation

The generated API docs are published at **https://jamieleecho.github.io/xmastree/**
and are rebuilt automatically whenever the `main` branch is updated.

To build them locally, the public headers are annotated for
[Doxygen](https://www.doxygen.nl/), which ships in the coco-dev image (>= 0.83)
alongside cmoc. From this directory inside the container:

```
make docs        # generates HTML into docs/ (gitignored)
```

The `Doxyfile` sets `WARN_IF_UNDOCUMENTED`/`WARN_NO_PARAMDOC`, so a clean run
doubles as a check that every public function, macro, and type is documented.

## Modules

| Module           | AppKit analog              | Notes |
|------------------|----------------------------|-------|
| `mv_undo_manager`| `NSUndoManager`            | one file per function |
| `mv_image`       | `NSImage`                  | |
| `mv_file_dialog` | `NSOpenPanel` / `NSSavePanel` | |
| `mv_app`         | `NSApplication`            | split: init / run / message_box / dialog / nop |
| `mv_document`    | `NSDocument`               | split per function |
| `mv_view`        | `NSView` (core)            | minimal view protocol (frame + draw + click) |
| `mv_image_grid`  | `NSMatrix`-ish             | single-select image-button grid (an `MVView`) |

Foundational headers (no AppKit analog): `mv_defs` (shared constants —
`MV_PATH_MAX`, `MV_INPATH`/`MV_OUTPATH`, the canonical `Flush()` declaration),
`mv_event` (`MVUiEvent` and friends, kept below `mv_app` so views don't depend
on it), and `mv_version` (framework version).

Include the whole API via `<mvkit/mvkit.h>`, or a single module via e.g.
`<mvkit/mv_document.h>`. The palette/chrome rules live in `mv_theme.h`.
