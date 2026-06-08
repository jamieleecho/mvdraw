/*
 * Using MVKit, stage 4: the image grid.
 *
 * MVImageGrid is a grid of single-select image buttons, and MVKit's first
 * "view" (it embeds an MVView). Load one image per button, lay them out with
 * mv_image_grid_init, route content-area clicks to it with
 * mv_view_dispatch_click, and get a callback when the selection changes.
 */
#include <stdio.h>

#include <cgfx.h>

#include <mvkit/mvkit.h>

#define NUM_BUTTONS 4
#define COLUMNS     2

static const MVTheme theme = { {
    0x00, 0x07, 0x38, 0x3f,
    0x24, 0x12, 0x09, 0x36,
    0x1b, 0x2d, 0x20, 0x10,
    0x08, 0x04, 0x07, 0x38
} };

mv_menu_none(grid_window, "grid");

static MVImageGrid grid;

/* image buffers holding each button's picture (image_ids[i] -> button i) */
static int image_ids[NUM_BUTTONS] = { 2, 3, 4, 5 };

static void grid_selected(MVImageGrid *g) {
    printf("Selected button %d\n", mv_image_grid_selected(g));
    Flush();
}

static void grid_pre_init(int argc, char **argv) {
    mv_app_set_theme(&theme);
    mv_image_init("grid");
    mv_image_load_resource("btn1.i09", image_ids[0]);
    mv_image_load_resource("btn2.i09", image_ids[1]);
    mv_image_load_resource("btn3.i09", image_ids[2]);
    mv_image_load_resource("btn4.i09", image_ids[3]);
}

static void grid_init(void) {
    _cgfx_bcolor(MV_OUTPATH, 0);
    _cgfx_clear(MV_OUTPATH);
    /* draws the grid immediately; the selected item gets an XOR highlight */
    mv_image_grid_init(&grid, 8, 8, NUM_BUTTONS, COLUMNS, image_ids, grid_selected);
    Flush();
}

static void grid_action(MVUiEvent *event) {
    if (event->event_type == MVUiEventType_MouseClick) {
        mv_view_dispatch_click(&grid.view, event);
    }
}

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &grid_window,
        grid_pre_init, grid_init, mv_app_menu_actions_nop,
        mv_app_refresh_menus_action_nop, grid_action);
}
