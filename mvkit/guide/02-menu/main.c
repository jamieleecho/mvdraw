/*
 * Using MVKit, stage 2: a menu and an action.
 *
 * Adds a Help menu with an "About..." item to the stage-1 window. Choosing it
 * runs about_action, which opens a modal message box.
 *
 * Menus are declared with the MV_MENU_* macros: a window's menus[] table lists
 * MV_MENU(...) rows, each pointing at a MIDSCR items[] table of MV_MENU_ITEM(...)
 * entries. At run time mv_app_run dispatches a chosen (menu, item) to the
 * matching row of the MVMenuItemAction table; the final {-1, -1, ...} row is the
 * catch-all for anything unhandled.
 */
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
    MV_MENU_ACTION_END            /* catch-all sentinel */
};

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &menu_window,
        mv_app_pre_init_nop, menu_init, menu_actions,
        mv_app_refresh_menus_action_nop, mv_app_event_nop);
}
