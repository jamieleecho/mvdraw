#ifndef _MVKIT_MV_IMAGE_GRID_H
#define _MVKIT_MV_IMAGE_GRID_H

#include <mvkit/mv_view.h>
#include <mvkit/mv_theme.h>

/**
 * @file
 * @brief MVImageGrid -- a grid of single-select image buttons (an MVView).
 */

/** @brief Default image width inside each button, in pixels. */
#define MV_IMAGE_GRID_ITEM_WIDTH  24
/** @brief Default image height inside each button, in pixels. */
#define MV_IMAGE_GRID_ITEM_HEIGHT 24
/** @brief Border thickness drawn around each button, in pixels. */
#define MV_IMAGE_GRID_ITEM_BORDER 1
/** @brief Default button border / highlight color (the theme's control fg). */
#define MV_IMAGE_GRID_DEFAULT_FG  MV_THEME_CONTROL_FG
/** @brief Default button background color (the theme's control bg). */
#define MV_IMAGE_GRID_DEFAULT_BG  MV_THEME_CONTROL_BG

/**
 * @brief A grid of single-select image buttons.
 *
 * The generalized form of xmastree's tool palette. It conforms to #MVView
 * (embedded first), so an app dispatches clicks to it via
 * mv_view_dispatch_click() and draws it via mv_view_draw(). Every button gets a
 * thin fg_color outline; the selected item gets a thick one. Borders are drawn
 * over the images, so the pictures keep their true colors.
 */
typedef struct MVImageGrid {
    MVView view;          /**< base; frame is computed from the layout in init */
    int num_items;        /**< number of buttons */
    int columns;          /**< buttons per row */
    int item_width;       /**< image width inside each button */
    int item_height;      /**< image height inside each button */
    int fg_color;         /**< button border / highlight color */
    int bg_color;         /**< background fill color */
    const int *image_ids; /**< num_items image-buffer ids (borrowed) */
    int selected;         /**< index of the currently selected item */
    void (*item_selected)(struct MVImageGrid *self);  /**< called on selection change (may be NULL) */
} MVImageGrid;

/**
 * @brief Initialize a grid and draw it immediately.
 *
 * Item size defaults to #MV_IMAGE_GRID_ITEM_WIDTH x #MV_IMAGE_GRID_ITEM_HEIGHT
 * and colors to #MV_IMAGE_GRID_DEFAULT_FG / #MV_IMAGE_GRID_DEFAULT_BG (set the
 * fields afterward to override); the view frame is computed from the layout.
 *
 * @param grid          the grid to initialize.
 * @param x             left edge, screen coords.
 * @param y             top edge, screen coords.
 * @param num_items     number of buttons.
 * @param columns       buttons per row.
 * @param image_ids     num_items image-buffer ids (borrowed; must outlive the grid).
 * @param item_selected called when the selection changes (may be NULL).
 */
extern void mv_image_grid_init(MVImageGrid *grid, int x, int y,
                               int num_items, int columns,
                               const int *image_ids,
                               void (*item_selected)(MVImageGrid *self));

/**
 * @brief Index of the currently selected item.
 * @param grid the grid.
 * @return the 0-based selected index.
 */
extern int mv_image_grid_selected(const MVImageGrid *grid);

/**
 * @brief Select an item (0-based), update the highlight, and fire item_selected.
 *
 * No-op if @p item is already selected.
 *
 * @param grid the grid.
 * @param item the 0-based item index to select.
 * @return false if @p item is out of range, true otherwise.
 */
extern bool mv_image_grid_select(MVImageGrid *grid, int item);

/**
 * @brief Show or hide the grid (and redraw).
 * @param grid       the grid.
 * @param is_visible true to show, false to hide.
 */
extern void mv_image_grid_set_visible(MVImageGrid *grid, bool is_visible);

#endif /* _MVKIT_MV_IMAGE_GRID_H */
