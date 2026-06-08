#ifndef _DRAW_VIEW_H_
#define _DRAW_VIEW_H_

#include <mvkit/mv_view.h>

#include "drawing.h"

/*
 * mvdraw's content view: an editable vector canvas. Embeds MVView first so the
 * app dispatches to it via mv_view_dispatch_click()/mv_view_draw(). It owns the
 * point-click-drag creation, selection, and corner-handle resize/move, drawing
 * XOR rubber-band previews during a drag and full-repainting on release.
 *
 * Edits are reported back to the app through on_add/on_edit so the app can
 * record them with its MVDocument/MVUndoManager (the view stays framework-light:
 * it only knows MVView + cgfx + the Drawing model).
 */

/* The toolbar tools. Index order matches the Tools image grid in mvdraw.c.
   TOOL_RECT..TOOL_LINE map to ShapeType via (tool - TOOL_RECT + SHAPE_RECT). */
typedef enum {
    TOOL_SELECT = 0,
    TOOL_RECT,
    TOOL_FRECT,
    TOOL_CIRCLE,
    TOOL_ELLIPSE,
    TOOL_LINE,
    TOOL_COUNT
} Tool;

typedef struct DrawView {
    MVView view;
    Drawing *drawing;

    /* current drawing attributes, driven by the toolbars */
    int tool;                 /* a Tool value */
    unsigned char pattern;    /* cgfx PAT_* */
    unsigned char color;      /* foreground palette register */
    unsigned char logic;      /* cgfx LOG_* */

    /* palette registers for chrome */
    int paper;                /* canvas background fill */
    int preview_color;        /* XOR rubber-band color (must have bit set) */
    int handle_color;         /* selection-handle fill */

    int selected;             /* index of the selected shape, or -1 */

    /* edit notifications (set by the app; may be NULL) */
    void (*on_add)(struct DrawView *self, int index);
    void (*on_edit)(struct DrawView *self, int index, void *record);
} DrawView;


extern void draw_view_init(DrawView *v, int x, int y, int width, int height,
                           Drawing *drawing);

/* Full repaint of the canvas (paper + every shape + handles). */
extern void draw_view_refresh(DrawView *v);

/* Attribute setters (called from toolbar item_selected callbacks). */
extern void draw_view_set_tool(DrawView *v, int tool);
extern void draw_view_set_pattern(DrawView *v, unsigned char pattern);
extern void draw_view_set_color(DrawView *v, unsigned char color);
extern void draw_view_set_logic(DrawView *v, unsigned char logic);

#endif /* _DRAW_VIEW_H_ */
