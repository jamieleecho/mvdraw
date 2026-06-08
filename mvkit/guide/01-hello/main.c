/*
 * Using MVKit, stage 1: the smallest real app.
 *
 * It opens a framed window (no menu bar) and uses the run-loop lifecycle. Of
 * the callbacks mv_app_run() takes, two run at startup:
 *
 *   pre_init  -- before the window exists (palette/image/model setup goes here)
 *   init      -- once, just after the window is set up
 *
 * Here init prints a loading message, pauses three seconds, then prints that it
 * is loaded -- showing that init runs to completion before the event loop
 * starts. Text printed to stdout lands in the app's window. The other callbacks
 * use the framework's no-op defaults; the window's close box quits the app.
 */
#include <stdio.h>
#include <unistd.h>          /* sleep */

#include <mvkit/mvkit.h>

mv_menu_none(hello_window, "hello");

static void hello_init(void) {
    printf("Loading...\n");
    Flush();
    sleep(3);
    printf("Loaded.\n");
    Flush();
}

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &hello_window,
        mv_app_pre_init_nop, hello_init, mv_app_menu_actions_nop,
        mv_app_refresh_menus_action_nop, mv_app_event_nop);
}
