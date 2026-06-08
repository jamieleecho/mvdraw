#include "mvkit/mv_app.h"

/* Default no-op handlers (see mv_app.h). They let a minimal app fill
   mv_app_run's callback slots explicitly rather than passing NULL. */


void mv_app_pre_init_nop(int argc, char **argv) {
    (void)argc;
    (void)argv;
}


void mv_app_init_nop(void) {
}


void mv_app_refresh_menus_action_nop(void) {
}


void mv_app_event_nop(MVUiEvent *event) {
}


void mv_app_menu_action_nop(MSRET *msinfo, int menuid, int itemno) {
}

/* Sentinel-only dispatch table: the menuid < 0 entry is the catch-all, so
   every menu selection is handled by the no-op. */
const MVMenuItemAction mv_app_menu_actions_nop[] = {
    MV_MENU_ACTION_END
};
