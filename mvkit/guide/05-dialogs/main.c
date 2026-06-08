/*
 * Using MVKit, stage 5: dialogs.
 *
 * A "Dialogs" menu whose items open the framework's modal dialogs:
 *   - mv_app_show_message_box  -- a message box (here Yes/No; also Info /
 *                                 Warning / Error / OkCancel)
 *   - mv_app_show_open_dialog  -- a file Open browser
 *   - mv_app_show_save_dialog  -- a file Save browser (offers "[new file]")
 * Each reports its result to the window.
 */
#include <stdio.h>

#include <mvkit/mvkit.h>

#define MN_DIALOGS 30

static MIDSCR dialog_items[] = {
    MV_MENU_ITEM("Message..."),
    MV_MENU_ITEM("Open..."),
    MV_MENU_ITEM("Save..."),
};

static MNDSCR menus[] = {
    MV_MENU("Dialogs", MN_DIALOGS, dialog_items),
};

mv_set_menus(dialogs_window, "dialogs", menus);

static void message_action(MSRET *msinfo, int menuid, int itemno) {
    MVMessageBoxResult result =
        mv_app_show_message_box("Proceed?", MVMessageBoxType_YesNo);
    printf("You chose %s\n", result == MVMessageBoxResult_Yes ? "Yes" : "No");
    Flush();
}

static void open_action(MSRET *msinfo, int menuid, int itemno) {
    char path[MV_PATH_MAX];
    if (mv_app_show_open_dialog(path, ".txt")) {
        printf("Open: %s\n", path);
    } else {
        printf("Open cancelled\n");
    }
    Flush();
}

static void save_action(MSRET *msinfo, int menuid, int itemno) {
    char path[MV_PATH_MAX];
    if (mv_app_show_save_dialog(path, ".txt")) {
        printf("Save: %s\n", path);
    } else {
        printf("Save cancelled\n");
    }
    Flush();
}

static const MVMenuItemAction menu_actions[] = {
    {MN_DIALOGS, 1, message_action},
    {MN_DIALOGS, 2, open_action},
    {MN_DIALOGS, 3, save_action},
    MV_MENU_ACTION_END
};

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &dialogs_window,
        mv_app_pre_init_nop, mv_app_init_nop, menu_actions,
        mv_app_refresh_menus_action_nop, mv_app_event_nop);
}
