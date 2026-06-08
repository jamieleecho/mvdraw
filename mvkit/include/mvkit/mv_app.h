#ifndef _MVKIT_MV_APP_H
#define _MVKIT_MV_APP_H

#include <stdlib.h>   /* size_t */
#include <cgfx.h>     /* WNDSCR, MSRET */
#include <stdbool.h>
#include <mvkit/mv_defs.h>
#include <mvkit/mv_event.h>   /* MVUiEvent */
#include <mvkit/mv_menu.h>    /* MV_MENU / mv_set_menus DSL */

/**
 * @file
 * @brief The application object, loosely analogous to AppKit's NSApplication.
 *
 * Palette/window setup, the main event loop, menu dispatch, and the built-in
 * modal dialogs (message box, file open/save).
 */

/**
 * @brief One row of the menu dispatch table passed to mv_app_run().
 *
 * When the menu item (@c menuid, @c itemno) is chosen, @c action is called with
 * the originating mouse record. Terminate the table with a sentinel whose
 * @c menuid is < 0; that sentinel's @c action becomes the catch-all for
 * unhandled selections.
 */
typedef struct {
    int menuid;   /**< menu id this row matches */
    int itemno;   /**< item number within the menu */
    void (*action)(MSRET *msinfo, int menuid, int itemno);   /**< invoked on selection */
} MVMenuItemAction;

/**
 * @brief Terminating sentinel row for a #MVMenuItemAction table.
 *
 * Expands to a catch-all row (@c menuid < 0) whose action is
 * mv_app_menu_action_nop, so unhandled selections are ignored. Use it as the
 * last entry instead of writing your own no-op:
 * @code
 * static const MVMenuItemAction menu_actions[] = {
 *     {MN_HELP, 1, about_action},
 *     MV_MENU_ACTION_END
 * };
 * @endcode
 */
#define MV_MENU_ACTION_END  { -1, -1, mv_app_menu_action_nop }

/**
 * @brief Style of a message box, selecting which buttons are shown.
 *
 * Info/Warning/Error show a lone OK; OkCancel and YesNo show two buttons.
 */
typedef enum {
    MVMessageBoxType_Info,       /**< informational; lone OK button */
    MVMessageBoxType_Warning,    /**< warning; lone OK button */
    MVMessageBoxType_Error,      /**< error; lone OK button */
    MVMessageBoxType_OkCancel,   /**< OK + Cancel buttons */
    MVMessageBoxType_YesNo       /**< Yes + No buttons */
} MVMessageBoxType;

/**
 * @brief Which button dismissed a message box.
 *
 * Ok and Yes share value 0 (the primary/default button); Cancel and No are the
 * secondary button.
 */
typedef enum {
    MVMessageBoxResult_Ok = 0,    /**< OK (primary) */
    MVMessageBoxResult_Cancel,    /**< Cancel (secondary) */
    MVMessageBoxResult_Yes = 0,   /**< Yes (primary) */
    MVMessageBoxResult_No         /**< No (secondary) */
} MVMessageBoxResult;

/**
 * @brief Install a screen palette.
 *
 * @c palette[0..num_colors) are cgfx color values for palette slots
 * 0..num_colors-1. Call before mv_app_run(). For a theme-aware alternative see
 * mv_app_set_theme().
 *
 * @param palette    array of cgfx color values.
 * @param num_colors number of palette slots to set.
 */
extern void mv_app_init(const int *palette, size_t num_colors);

/**
 * @brief Open the window, enter the main event loop, and run until the program
 *        exits.
 *
 * In practice this does not return; the int return type lets main() be a single
 * `return mv_app_run(...);`. Prefer the #mv_app_run / #mv_app_run_with_scrollbars
 * macros, which bake in the window type; call this directly only to pass another
 * cgfx window type. Callbacks may be NULL, or pass the mv_app_*_nop defaults.
 *
 * @param window_type          a cgfx WT_ value (WT_FWIN = framed window,
 *                             WT_FSWIN = framed window with scrollbars).
 * @param argc                 process argument count.
 * @param argv                 process argument vector.
 * @param mywindow             the window descriptor (see mv_set_menus()).
 * @param pre_init             run first, before window setup, with argc/argv; do
 *                             palette/image/model setup here, and open a document
 *                             named on the command line if wanted.
 * @param init                 run once after the window is set up.
 * @param menu_actions         menu dispatch table (see #MVMenuItemAction). The
 *                             window close box quits by default; supply an
 *                             MN_CLOS entry to intercept it (e.g. save first).
 * @param refresh_menus_action run before redrawing the menu bar; update item
 *                             enable/disable state here.
 * @param application_action   called with each key press and content-area mouse
 *                             click for the app to handle.
 * @return the process exit status (in practice does not return).
 */
extern int mv_app_run_typed(
    int window_type,
    int argc, char **argv,
    WNDSCR *mywindow,
    void (*pre_init)(int argc, char **argv),
    void (*init)(void),
    const MVMenuItemAction *menu_actions,
    void (*refresh_menus_action)(void),
    void (*application_action)(MVUiEvent *event)
);

/**
 * @brief Run an app in a plain framed window (WT_FWIN).
 *
 * Zero-cost convenience over mv_app_run_typed() -- a macro, so there is no
 * wrapper stack frame (which, since mv_app_run_typed never returns, would
 * otherwise persist for the whole run).
 *
 * @param argc                 process argument count.
 * @param argv                 process argument vector.
 * @param window               the window descriptor.
 * @param pre_init             pre-window-setup callback.
 * @param init                 post-window-setup callback.
 * @param menu_actions         menu dispatch table.
 * @param refresh_menus_action menu-refresh callback.
 * @param application_action   key/click handler.
 */
#define mv_app_run(argc, argv, window, pre_init, init, menu_actions, refresh_menus_action, application_action) \
    mv_app_run_typed(WT_FWIN, (argc), (argv), (window), (pre_init), (init), \
                     (menu_actions), (refresh_menus_action), (application_action))

/**
 * @brief Run an app in a framed window with scrollbars (WT_FSWIN).
 *
 * Same parameters as #mv_app_run.
 *
 * @param argc                 process argument count.
 * @param argv                 process argument vector.
 * @param window               the window descriptor.
 * @param pre_init             pre-window-setup callback.
 * @param init                 post-window-setup callback.
 * @param menu_actions         menu dispatch table.
 * @param refresh_menus_action menu-refresh callback.
 * @param application_action   key/click handler.
 */
#define mv_app_run_with_scrollbars(argc, argv, window, pre_init, init, menu_actions, refresh_menus_action, application_action) \
    mv_app_run_typed(WT_FSWIN, (argc), (argv), (window), (pre_init), (init), \
                     (menu_actions), (refresh_menus_action), (application_action))

/**
 * @brief Request that the menu bar be rebuilt before the next event is
 *        processed.
 *
 * Triggers another call to the run loop's refresh_menus_action. Call after
 * changing what menu items should be enabled.
 */
extern void mv_app_refresh_menubar(void);

/**
 * @brief Show a modal message box and return the button the user chose.
 * @param message text to show; may embed CR/LF to wrap onto up to three lines.
 * @param type    which buttons to show.
 * @return the button that dismissed the box.
 */
extern MVMessageBoxResult mv_app_show_message_box(const char *message, MVMessageBoxType type);

/**
 * @brief Show a modal Open file browser, centered on screen.
 *
 * Lists files whose name ends with @p ext (including its dot, e.g. ".xmt"; NULL
 * lists all). On confirm, copies the chosen name into @p path.
 *
 * @param path a buffer of at least #MV_PATH_MAX bytes to receive the choice.
 * @param ext  extension filter including its dot, or NULL to list all.
 * @return @p path on confirm, or NULL if cancelled.
 */
extern char *mv_app_show_open_dialog(char *path, const char *ext);

/**
 * @brief Show a modal Save file browser, centered on screen.
 *
 * Like mv_app_show_open_dialog(), but also offers a "[new file]" entry and
 * appends @p ext to a typed name that lacks it.
 *
 * @param path a buffer of at least #MV_PATH_MAX bytes to receive the choice.
 * @param ext  extension appended to a name that lacks it, or NULL.
 * @return @p path on confirm, or NULL if cancelled.
 */
extern char *mv_app_show_save_dialog(char *path, const char *ext);


/* Default no-op handlers, so a minimal app can fill mv_app_run's callback slots
   explicitly instead of passing NULL. */

/** @brief No-op default pre_init handler.
 *  @param argc unused. @param argv unused. */
extern void mv_app_pre_init_nop(int argc, char **argv);
/** @brief No-op default init handler. */
extern void mv_app_init_nop(void);
/** @brief No-op default refresh_menus_action handler. */
extern void mv_app_refresh_menus_action_nop(void);
/** @brief No-op default application_action handler.
 *  @param event unused. */
extern void mv_app_event_nop(MVUiEvent *event);
/** @brief No-op default menu action handler.
 *  @param msinfo unused. @param menuid unused. @param itemno unused. */
extern void mv_app_menu_action_nop(MSRET *msinfo, int menuid, int itemno);
/** @brief A sentinel-only menu dispatch table (every selection is ignored). */
extern const MVMenuItemAction mv_app_menu_actions_nop[];


/* The menu-bar / window-descriptor declaration DSL (MV_MENU, MV_MENU_ITEM,
   mv_set_menus, ...) lives in <mvkit/mv_menu.h>, included above. */

#endif /* _MVKIT_MV_APP_H */
