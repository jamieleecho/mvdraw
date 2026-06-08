#ifndef _MVKIT_H
#define _MVKIT_H

/**
 * @file
 * @brief MVKit umbrella header -- include <mvkit/mvkit.h> for the whole API.
 *
 * MVKit is an opinionated framework for building Multi-Vue applications on the
 * Tandy Color Computer 3 under NitrOS-9, loosely inspired by Apple's AppKit.
 *
 * Modules (AppKit analog in parentheses):
 *   - mv_defs, mv_version          -- foundational constants / version
 *   - mv_event                     -- input events (key / mouse)
 *   - mv_theme                     -- palette / window-chrome theme
 *   - mv_menu                      -- menu-bar / window-descriptor DSL
 *   - mv_undo_manager  (NSUndoManager)
 *   - mv_image         (NSImage)
 *   - mv_file_dialog   (NSOpenPanel / NSSavePanel)
 *   - mv_app           (NSApplication)
 *   - mv_document      (NSDocument)
 *   - mv_view          (NSView core)
 *   - mv_image_grid    (NSMatrix-ish image-button grid)
 */

#include <mvkit/mv_defs.h>
#include <mvkit/mv_event.h>
#include <mvkit/mv_version.h>
#include <mvkit/mv_theme.h>
#include <mvkit/mv_menu.h>
#include <mvkit/mv_undo_manager.h>
#include <mvkit/mv_image.h>
#include <mvkit/mv_file_dialog.h>
#include <mvkit/mv_app.h>
#include <mvkit/mv_document.h>
#include <mvkit/mv_view.h>
#include <mvkit/mv_image_grid.h>

#endif /* _MVKIT_H */
