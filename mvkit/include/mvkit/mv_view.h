#ifndef _MVKIT_MV_VIEW_H
#define _MVKIT_MV_VIEW_H

#include <stdbool.h>
#include <mvkit/mv_event.h>   /* MVUiEvent */

/**
 * @file
 * @brief MVView -- MVKit's minimal view protocol.
 *
 * Loosely NSView's core: a frame, a draw entry point, and click handling.
 * Concrete views (e.g. MVImageGrid, or an app's own content view) embed an
 * MVView as their FIRST member, so a `ConcreteView *` can be used as an
 * `MVView *`. There is no view hierarchy or automatic event routing yet; an app
 * keeps its own list of views and dispatches to them (see
 * mv_view_dispatch_click()).
 */

/**
 * @brief The view protocol: a frame, visibility, a draw entry point, and a
 *        click handler. Embed as the FIRST member of a concrete view.
 */
typedef struct MVView {
    int x;             /**< left edge, screen coords */
    int y;             /**< top edge, screen coords */
    int width;         /**< frame width */
    int height;        /**< frame height */
    bool is_visible;   /**< when false, the view neither draws nor handles clicks */
    void (*draw)(struct MVView *self);                       /**< redraw the view */
    bool (*handle_click)(struct MVView *self, MVUiEvent *e);  /**< hit-test + handle; true if consumed */
} MVView;

/**
 * @brief Whether (x, y) lies within the view's frame.
 *
 * A convenience for view implementations that want a simple rectangular
 * hit-test.
 *
 * @param view the view.
 * @param x    x coordinate, screen coords.
 * @param y    y coordinate, screen coords.
 * @return true if the point is inside the frame.
 */
extern bool mv_view_contains_point(const MVView *view, int x, int y);

/**
 * @brief Draw the view if it is visible and has a draw function.
 * @param view the view.
 */
extern void mv_view_draw(MVView *view);

/**
 * @brief Offer a click to the view.
 *
 * If the view is visible and has a handler, call it and return whether it
 * consumed the event; otherwise return false. The handler is responsible for
 * its own hit-testing (views may want non-rectangular bounds).
 *
 * @param view  the view.
 * @param event the click event.
 * @return true if the view consumed the event.
 */
extern bool mv_view_dispatch_click(MVView *view, MVUiEvent *event);

#endif /* _MVKIT_MV_VIEW_H */
