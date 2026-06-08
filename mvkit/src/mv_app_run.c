#include <cgfx.h>
#include <signal.h>
#include <unistd.h>   /* read, sleep */

#include "mvkit/mv_app.h"


#define KEY_SIG  11  /* signal number for key interrupts */

#define MOUSE_UPDATE_PERIOD    3  /* check every 3 interrupts */
#define MOUSE_TIMEOUT_PERIOD  10  /* timeout every 10 interrupts */
#define MOUSE_FOLLOW           1  /* update gc immediately */
#define MOUSE_SIG             10  /* signal number for mouse interrupts */


static char sigcode = 0;
static void sighandler(int signal) {
    sigcode = (char)signal;
}


static void run_event_loop(MVUiEvent *event) {
    int local_sig;
    bool first = true;

    while(true) {
        sigcode = 0;
        do {
            _cgfx_ss_mssig(MV_OUTPATH, MOUSE_SIG);
            do {
                sigcode = 0;
                _cgfx_ss_mssig(MV_OUTPATH, MOUSE_SIG);
            }
            while (first && sigcode);
            first = false;
            _ss_ssig(MV_OUTPATH, KEY_SIG);
            sleep(1);
        } while (sigcode == 0);
        local_sig = sigcode;

        if (local_sig == KEY_SIG) {
            char c;
            if (!read(MV_INPATH, &c, 1)) {
                c = 0;
            }
            event->event_type = MVUiEventType_KeyPress;
            event->info.key.character = c;
            return;
        } else if (local_sig == MOUSE_SIG) {
            event->event_type = MVUiEventType_MouseClick;
            _cgfx_gs_mouse(MV_OUTPATH, &event->info.mouse);
            return;
        }
    }
}


static void echo_sw(path_id path, char on) {
    struct sgbuf options;
    _gs_opt(path, &options);
    options.sg_echo = on;
    _ss_opt(path, &options);
}


static int refresh_menu_bar;


void mv_app_refresh_menubar(void) {
    refresh_menu_bar = true;
}


int mv_app_run_typed(int window_type, int argc, char **argv, WNDSCR *mywindow,
                     void (*pre_init)(int argc, char **argv),
                     void (*init)(void),
                     const MVMenuItemAction *menu_actions,
                     void (*refresh_menus_action)(void),
                     void (*application_action)(MVUiEvent *event)) {
    int itemno, menuid, ii;
    MVMenuItemAction const * menu_item_action;
    MVUiEvent event;

    if (pre_init) {
        pre_init(argc, argv);
    }

    echo_sw(MV_OUTPATH, 0);
    intercept(sighandler);

    _cgfx_curoff(MV_OUTPATH);
    _cgfx_tcharsw(MV_OUTPATH, false);
    _cgfx_scalesw(MV_OUTPATH, false);
    _cgfx_setgc(MV_OUTPATH, GRP_PTR, PTR_ARR);
    _cgfx_ss_mouse(MV_OUTPATH, MOUSE_UPDATE_PERIOD, MOUSE_TIMEOUT_PERIOD, MOUSE_FOLLOW);

    int err = _cgfx_ss_wnset(0, window_type, mywindow);
    if (init) {
        init();
    }
    refresh_menu_bar = true;

    while(true) {
        if (refresh_menu_bar) {
            if (refresh_menus_action) {
                refresh_menus_action();
            }

            _cgfx_ss_umbar(MV_OUTPATH);
            refresh_menu_bar = false;
        }

        run_event_loop(&event);

        if (event.event_type == MVUiEventType_KeyPress) {
            if (application_action) {
                application_action(&event);
            }
            continue;
        }

        /* must be a mouse event */

        if (event.info.mouse.pt_valid == 0) {
            continue;
        }

        if (event.info.mouse.pt_stat == WR_CNTRL) {
            /* handle menus */
            _cgfx_gs_mnsel(MV_OUTPATH, &itemno, &menuid);
            for (ii=0; menu_actions[ii].menuid >= 0; ++ii) {
                menu_item_action = menu_actions + ii;
                if (menu_item_action->menuid == menuid &&
                    menu_item_action->itemno == itemno) {
                    menu_item_action->action(&event.info.mouse, menuid, itemno);
                    break;
                }
            }

            /* unhandled menu */
            if (menu_actions[ii].menuid < 0) {
                if (menuid == MN_CLOS) {
                    /* Default close-box behavior: quit. An app that wants to
                       intercept the close (e.g. to prompt to save) supplies its
                       own MN_CLOS entry, which matches above and pre-empts this. */
                    exit(0);
                }
                menu_actions[ii].action(&event.info.mouse, menuid, itemno);
            }

            continue;
        }

        /* handle content window events */
        if (event.info.mouse.pt_stat == WR_CNTNT) {
            if (application_action) {
                application_action(&event);
            }
        }
    }

    return 0;   /* not reached: the event loop runs until the process exits */
}
