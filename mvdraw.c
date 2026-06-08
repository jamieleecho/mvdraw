#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <cgfx.h>
#include <mvkit/mvkit.h>

#include "drawing.h"
#include "draw_view.h"
#include "version.h"


/*
 * mvdraw -- a vector drawing program for Multi-Vue, built with MVKit.
 *
 * Structure mirrors the xmastree example: a theme, a menu bar, image-grid
 * toolbars, an MVDocument wrapping the data model, and a content view. Here the
 * content view (draw_view.c) is an editable vector canvas, and there are four
 * toolbars: shape tools, fill patterns, color, and logic-drawing mode.
 */

/* App-chosen menu ids (cgfx reserves the low numbers; MN_FILE/MN_EDIT/MN_STYL
   are predefined, MN_HELP is ours). */
#define MN_HELP 30

/* Two-color (type 5) theme. Only palette registers 0 and 1 are displayed at
   1bpp; cowin/dialog chrome that references regs 2-3 collapses to bit0, so even
   registers read black and odd read white. We keep 0=black,1=white (and mirror
   into 2-3) so ink/paper/dialogs all resolve correctly on the monochrome
   screen. Registers 4-15 are unused at this depth. */
static const MVTheme theme = { {
    0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3f,
    0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3f, 0x00, 0x3f
} };

#define WINDOW_BACKGROUND 0   /* black desktop behind the white panels */

/* Palette registers used as colors. */
#define COLOR_BLACK 0
#define COLOR_WHITE 1


/* ---- layout (pixels, window-relative) ----------------------------------- */

#define BTN (MV_IMAGE_GRID_ITEM_WIDTH + MV_IMAGE_GRID_ITEM_BORDER)   /* 25 */

#define TOOLS_X     2
#define TOOLS_Y     2
#define TOOLS_COLS  2
#define TOOLS_ROWS  3

#define LOGIC_X     2
#define LOGIC_Y     (TOOLS_Y + TOOLS_ROWS * BTN + 3)
#define LOGIC_COLS  2

#define COLOR_X     (TOOLS_X + LOGIC_COLS * BTN + 6)
#define COLOR_Y     (184 - BTN - 1)        /* a single row along the bottom */
#define COLOR_COLS  2

#define PAT_X       (COLOR_X + COLOR_COLS * BTN + 6)
#define PAT_Y       (184 - BTN - 1)        /* a single row along the bottom */
#define PAT_COLS    9

#define CANVAS_X    COLOR_X
#define CANVAS_Y    2
#define CANVAS_W    (624 - CANVAS_X)
#define CANVAS_H    (PAT_Y - 4 - CANVAS_Y)


/* ---- counts and image buffers ------------------------------------------- */

#define NUM_TOOLS    6
#define NUM_PATTERNS 9
#define NUM_COLORS   2
#define NUM_LOGICS   4

/* get/put buffer numbers, one per toolbar image (2..22). */
static int tool_ids[NUM_TOOLS]    = { 2, 3, 4, 5, 6, 7 };
static int pat_ids[NUM_PATTERNS]  = { 8, 9, 10, 11, 12, 13, 14, 15, 16 };
static int color_ids[NUM_COLORS]  = { 17, 18 };
static int logic_ids[NUM_LOGICS]  = { 19, 20, 21, 22 };


/* ---- model, document, views --------------------------------------------- */

static Drawing drawing;
static MVDocument doc;

static MVImageGrid tools_grid;
static MVImageGrid patterns_grid;
static MVImageGrid colors_grid;
static MVImageGrid logic_grid;
static DrawView canvas;


/* ---- menus -------------------------------------------------------------- */

typedef enum { FileMenuIndex_Save = 3 } FileMenuIndex;

static MIDSCR file_menu_items[] = {
    MV_MENU_ITEM("New"),
    MV_MENU_SEPARATOR,
    MV_MENU_ITEM("Open..."),
    MV_MENU_ITEM("Save"),
    MV_MENU_ITEM("Save As..."),
    MV_MENU_SEPARATOR,
    MV_MENU_ITEM("Exit"),
};

typedef enum { EditMenuIndex_Undo = 0, EditMenuIndex_Delete = 2 } EditMenuIndex;

static MIDSCR edit_menu_items[] = {
    MV_MENU_ITEM("Undo"),
    MV_MENU_SEPARATOR,
    MV_MENU_ITEM("Delete"),
};

static MIDSCR help_menu_items[] = {
    MV_MENU_ITEM("About..."),
};

static MNDSCR menus[] = {
    MV_MENU("File", MN_FILE, file_menu_items),
    MV_MENU("Edit", MN_EDIT, edit_menu_items),
    MV_MENU("Help", MN_HELP, help_menu_items),
};

mv_set_menus(mywindow, "mvdraw", menus);

/* ---- menu actions ------------------------------------------------------- */

static void exit_action(MSRET *msinfo, int menuid, int itemno) {
    if (mv_document_close(&doc)) {
        exit(0);
    }
}

static void new_action(MSRET *msinfo, int menuid, int itemno) {
    if (mv_document_new(&doc)) {
        canvas.selected = -1;
        draw_view_refresh(&canvas);
    }
}

static void open_action(MSRET *msinfo, int menuid, int itemno) {
    if (mv_document_open(&doc)) {
        canvas.selected = -1;
        draw_view_refresh(&canvas);
    }
}

static void save_action(MSRET *msinfo, int menuid, int itemno) {
    mv_document_save(&doc);
}

static void save_as_action(MSRET *msinfo, int menuid, int itemno) {
    mv_document_save_as(&doc);
}

static void about_action(MSRET *msinfo, int menuid, int itemno) {
    mv_app_show_message_box("mvdraw v" APP_VERSION "\r(C) 2026 Jamie Cho",
                            MVMessageBoxType_Info);
}

static void undo_action(MSRET *msinfo, int menuid, int itemno) {
    if (mv_document_undo(&doc)) {
        draw_view_refresh(&canvas);
    }
}

static void delete_action(MSRET *msinfo, int menuid, int itemno) {
    int idx = canvas.selected;
    if (idx >= 0 && idx < drawing_count(&drawing)) {
        void *record = drawing_delete_shape(&drawing, idx);
        canvas.selected = -1;
        if (record) {
            MVUndoItem item = { drawing_undo_reinsert, record };
            mv_document_make_change(&doc, &item);
        }
        draw_view_refresh(&canvas);
        mv_app_refresh_menubar();   /* Delete is now disabled (nothing selected) */
    }
}

static MVMenuItemAction menu_actions[] = {
    {MN_CLOS, 1, exit_action},
    {MN_FILE, 1, new_action},
    {MN_FILE, 3, open_action},
    {MN_FILE, 4, save_action},
    {MN_FILE, 5, save_as_action},
    {MN_FILE, 7, exit_action},
    {MN_EDIT, 1, undo_action},
    {MN_EDIT, 3, delete_action},
    {MN_HELP, 1, about_action},
    MV_MENU_ACTION_END
};


/* ---- toolbar callbacks -------------------------------------------------- */

static void tools_selected(MVImageGrid *g) {
    draw_view_set_tool(&canvas, mv_image_grid_selected(g));
}

static void patterns_selected(MVImageGrid *g) {
    /* grid index 0..8 == cgfx PAT_SLD..PAT_BDOT */
    draw_view_set_pattern(&canvas, (unsigned char)mv_image_grid_selected(g));
}

static void colors_selected(MVImageGrid *g) {
    /* index 0 == black (reg 0), 1 == white (reg 1) */
    draw_view_set_color(&canvas, (unsigned char)mv_image_grid_selected(g));
}

static void logic_selected(MVImageGrid *g) {
    /* grid index 0..3 == cgfx LOG_NONE..LOG_XOR */
    draw_view_set_logic(&canvas, (unsigned char)mv_image_grid_selected(g));
}


/* ---- edit notifications: record changes with the document/undo manager --- */

static void canvas_on_add(DrawView *v, int index) {
    MVUndoItem item = { drawing_undo_remove_last, &drawing };
    mv_document_make_change(&doc, &item);
}

static void canvas_on_edit(DrawView *v, int index, void *record) {
    MVUndoItem item = { drawing_undo_restore, record };
    mv_document_make_change(&doc, &item);
}


/* ---- event handling ----------------------------------------------------- */

static void handle_key_event(MVUiEvent *event) {
    char c = event->info.key.character;
    if (c >= '1' && c <= '6') {
        mv_image_grid_select(&tools_grid, c - '1');
    } else if (c == '\x1A') {            /* Ctrl-Z */
        undo_action((MSRET *)NULL, -1, -1);
    }
}

static void handle_click_event(MVUiEvent *event) {
    if (mv_view_dispatch_click(&tools_grid.view, event)) return;
    if (mv_view_dispatch_click(&patterns_grid.view, event)) return;
    if (mv_view_dispatch_click(&colors_grid.view, event)) return;
    if (mv_view_dispatch_click(&logic_grid.view, event)) return;
    /* A canvas click can change the selection, so refresh the menu bar to keep
       Edit > Delete's enabled state in sync. */
    mv_view_dispatch_click(&canvas.view, event);
    mv_app_refresh_menubar();
}

static void mvdraw_action(MVUiEvent *event) {
    switch (event->event_type) {
        case MVUiEventType_KeyPress:
            handle_key_event(event);
            break;
        case MVUiEventType_MouseClick:
            handle_click_event(event);
            break;
    }
}


/* ---- lifecycle ---------------------------------------------------------- */

static void load_images(const char **names, const int *ids, int n) {
    int i;
    for (i = 0; i < n; ++i) {
        mv_image_load_resource(names[i], ids[i]);
    }
}

static const char *tool_images[NUM_TOOLS] = {
    "tool_select.i09", "tool_rect.i09", "tool_frect.i09",
    "tool_circle.i09", "tool_ellipse.i09", "tool_line.i09"
};
static const char *pat_images[NUM_PATTERNS] = {
    "pat0.i09", "pat1.i09", "pat2.i09", "pat3.i09", "pat4.i09",
    "pat5.i09", "pat6.i09", "pat7.i09", "pat8.i09"
};
static const char *color_images[NUM_COLORS] = {
    "col_black.i09", "col_white.i09"
};
static const char *logic_images[NUM_LOGICS] = {
    "logic_none.i09", "logic_and.i09", "logic_or.i09", "logic_xor.i09"
};

static void mvdraw_pre_init(int argc, char **argv) {
    if (argc > 2) {
        exit(1);
    }

    _cgfx_setgc(MV_OUTPATH, GRP_PTR, PTR_SLP);   /* hourglass while loading */
    mv_app_set_theme(&theme);
    mv_image_init("mvdraw");

    load_images(tool_images, tool_ids, NUM_TOOLS);
    load_images(pat_images, pat_ids, NUM_PATTERNS);
    load_images(color_images, color_ids, NUM_COLORS);
    load_images(logic_images, logic_ids, NUM_LOGICS);
    Flush();

    drawing_new(&drawing, NULL);
    mv_document_init(
        &doc,
        (argc == 2) ? argv[1] : (char *)0,
        "drawing",
        ".mvd",
        &drawing,
        (int (*)(void *, const char *))drawing_new,
        (int (*)(void *, const char *))drawing_open,
        (int (*)(void *, const char *))drawing_save
    );

    if (argc == 2) {
        drawing_open(&drawing, argv[1]);
        mv_document_opened(&doc);
    }
}

static void mvdraw_init(void) {
    assert(strncmp(file_menu_items[FileMenuIndex_Save]._mittl, "Save", 5) == 0);
    assert(strncmp(edit_menu_items[EditMenuIndex_Undo]._mittl, "Undo", 5) == 0);

    /* black border/highlight over white button backgrounds, for the
       white-on-paper look. */
    mv_image_grid_init_ex(&tools_grid, TOOLS_X, TOOLS_Y, NUM_TOOLS, TOOLS_COLS,
                          tool_ids, tools_selected, COLOR_BLACK, COLOR_WHITE, true);
    mv_image_grid_init_ex(&patterns_grid, PAT_X, PAT_Y, NUM_PATTERNS, PAT_COLS,
                          pat_ids, patterns_selected, COLOR_BLACK, COLOR_WHITE, true);
    mv_image_grid_init_ex(&colors_grid, COLOR_X, COLOR_Y, NUM_COLORS, COLOR_COLS,
                          color_ids, colors_selected, COLOR_BLACK, COLOR_WHITE, true);
    mv_image_grid_init_ex(&logic_grid, LOGIC_X, LOGIC_Y, NUM_LOGICS, LOGIC_COLS,
                          logic_ids, logic_selected, COLOR_BLACK, COLOR_WHITE, true);

    draw_view_init(&canvas, CANVAS_X, CANVAS_Y, CANVAS_W, CANVAS_H, &drawing);
    canvas.on_add = canvas_on_add;
    canvas.on_edit = canvas_on_edit;
    draw_view_refresh(&canvas);

    /* The Select (pointer) tool is the default: the tools grid starts with
       item 0 selected/highlighted, and draw_view_init defaults to TOOL_SELECT. */

    Flush();
}

static void mvdraw_refresh_menus_action(void) {
    mv_menu_item_set_enabled(file_menu_items, FileMenuIndex_Save,
                             mv_document_is_dirty(&doc));
    mv_menu_item_set_enabled(edit_menu_items, EditMenuIndex_Undo,
                             mv_document_can_undo(&doc));
    mv_menu_item_set_enabled(edit_menu_items, EditMenuIndex_Delete,
                             canvas.selected >= 0 &&
                             canvas.selected < drawing_count(&drawing));
}

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &mywindow, mvdraw_pre_init, mvdraw_init,
                      menu_actions, mvdraw_refresh_menus_action, mvdraw_action);
}
