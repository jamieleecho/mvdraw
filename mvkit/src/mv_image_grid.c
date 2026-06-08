#include <cgfx.h>
#include <stdbool.h>

#include <mvkit/mv_defs.h>
#include <mvkit/mv_image.h>
#include "mvkit/mv_image_grid.h"


static int button_width(const MVImageGrid *grid) {
    return grid->item_width + MV_IMAGE_GRID_ITEM_BORDER;
}

static int button_height(const MVImageGrid *grid) {
    return grid->item_height + MV_IMAGE_GRID_ITEM_BORDER;
}


static void image_grid_clear(MVImageGrid *grid) {
    _cgfx_lset(MV_OUTPATH, LOG_NONE);
    _cgfx_fcolor(MV_OUTPATH, grid->bg_color);
    _cgfx_setdptr(MV_OUTPATH, grid->view.x, grid->view.y);
    _cgfx_rbar(MV_OUTPATH, grid->view.width, grid->view.height);
}


static void image_grid_draw_image(MVImageGrid *grid, int item) {
    int bw = button_width(grid);
    int bh = button_height(grid);
    int x = grid->view.x + (item % grid->columns) * bw;
    int y = grid->view.y + (item / grid->columns) * bh;
    mv_image_draw(grid->image_ids[item], x + 1, y + 1);
}


/* Draw item's border in fg_color: a 1px box around every button (the separator
   outline), plus a second box one pixel in for the selected item, giving it a
   thick highlight. Borders are drawn over the images, so they're always visible;
   the image (true colors) shows inside. */
static void image_grid_draw_border(MVImageGrid *grid, int item, bool thick) {
    int bw = button_width(grid);
    int bh = button_height(grid);
    int x = grid->view.x + (item % grid->columns) * bw;
    int y = grid->view.y + (item / grid->columns) * bh;
    _cgfx_lset(MV_OUTPATH, LOG_NONE);
    _cgfx_fcolor(MV_OUTPATH, grid->fg_color);
    _cgfx_setdptr(MV_OUTPATH, x, y);
    _cgfx_rbox(MV_OUTPATH, bw, bh);
    if (thick) {
        _cgfx_setdptr(MV_OUTPATH, x + 1, y + 1);
        _cgfx_rbox(MV_OUTPATH, bw - 2, bh - 2);
    }
}


static void image_grid_draw(MVView *view) {
    MVImageGrid *grid = (MVImageGrid *)view;

    image_grid_clear(grid);
    if (!view->is_visible) {
        return;
    }
    for (int item = 0; item < grid->num_items; ++item) {   /* all images first */
        image_grid_draw_image(grid, item);
    }
    for (int item = 0; item < grid->num_items; ++item) {   /* then the thin borders */
        image_grid_draw_border(grid, item, false);
    }
    image_grid_draw_border(grid, grid->selected, true);    /* thick border on selected */
    Flush();
}


static bool image_grid_handle_click(MVView *view, MVUiEvent *event) {
    MVImageGrid *grid = (MVImageGrid *)view;
    int x, y;

    if (event->event_type != MVUiEventType_MouseClick) {
        return false;
    }
    x = event->info.mouse.pt_wrx;
    y = event->info.mouse.pt_wry;
    if (!mv_view_contains_point(view, x, y)) {
        return false;
    }
    x = (x - view->x) / button_width(grid);
    y = (y - view->y) / button_height(grid);
    return mv_image_grid_select(grid, y * grid->columns + x);
}


void mv_image_grid_init_ex(MVImageGrid *grid, int x, int y,
                           int num_items, int columns,
                           const int *image_ids,
                           void (*item_selected)(MVImageGrid *self),
                           int fg_color, int bg_color, bool is_visible) {
    int rows = (num_items + columns - 1) / columns;

    grid->num_items = num_items;
    grid->columns = columns;
    grid->item_width = MV_IMAGE_GRID_ITEM_WIDTH;
    grid->item_height = MV_IMAGE_GRID_ITEM_HEIGHT;
    grid->fg_color = fg_color;
    grid->bg_color = bg_color;
    grid->image_ids = image_ids;
    grid->selected = 0;
    grid->item_selected = item_selected;

    grid->view.x = x;
    grid->view.y = y;
    grid->view.width = columns * button_width(grid);
    grid->view.height = rows * button_height(grid);
    grid->view.is_visible = is_visible;
    grid->view.draw = image_grid_draw;
    grid->view.handle_click = image_grid_handle_click;

    image_grid_draw(&grid->view);
}


int mv_image_grid_selected(const MVImageGrid *grid) {
    return grid->selected;
}


bool mv_image_grid_select(MVImageGrid *grid, int item) {
    int previous;

    if ((item < 0) || (item >= grid->num_items)) {
        return false;
    }
    if (item == grid->selected) {
        return true;
    }
    previous = grid->selected;
    grid->selected = item;
    image_grid_draw_image(grid, previous);          /* redraw old selection's image */
    image_grid_draw_border(grid, previous, false);  /* ...with its thin border */
    image_grid_draw_border(grid, item, true);       /* thick border on the new selection */
    Flush();
    if (grid->item_selected) {
        grid->item_selected(grid);
    }
    return true;
}


void mv_image_grid_set_visible(MVImageGrid *grid, bool is_visible) {
    grid->view.is_visible = is_visible;
    image_grid_draw(&grid->view);
    Flush();
}
