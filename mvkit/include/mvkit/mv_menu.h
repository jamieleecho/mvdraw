#ifndef _MVKIT_MV_MENU_H
#define _MVKIT_MV_MENU_H

#include <cgfx.h>     /* WNDSCR, MNDSCR, MIDSCR, MN_ENBL, MN_DSBL */

/**
 * @file
 * @brief Declarative DSL for cgfx menu-bar / window descriptors.
 *
 * These macros hide the reserved bytes and positional layout of WNDSCR /
 * MNDSCR / MIDSCR so an app declares its menus and window readably:
 *
 * @code
 * static MIDSCR file_items[] = {
 *     MV_MENU_ITEM("New"), MV_MENU_SEPARATOR, MV_MENU_ITEM("Open..."),
 * };
 * static MNDSCR menus[] = { MV_MENU("File", MN_FILE, file_items), ... };
 * mv_set_menus(mywindow, "myapp", menus);
 * @endcode
 *
 * Convention: UPPER_CASE macros are value/initializer macros (use them inside
 * an array initializer); lower_case macros declare a static variable.
 */

/** @brief Absolute minimum window width the window manager allows. */
#define MV_WINDOW_WIDTH_FLOOR   10
/** @brief Absolute minimum window height the window manager allows. */
#define MV_WINDOW_HEIGHT_FLOOR  5

/** @brief Default minimum window width, used by the no-suffix window macros. */
#define MV_WINDOW_MIN_WIDTH  40
/** @brief Default minimum window height, used by the no-suffix window macros. */
#define MV_WINDOW_MIN_HEIGHT 24
/** @brief WNDSCR sync marker ($C0C0); filled in by the window macros. */
#define MV_WINDOW_SYNC       0xC0C0

/**
 * @brief The dropdown width implied by a dash string (its length).
 *
 * cowin draws an item's title at its own length (it does NOT clip to the menu
 * width), so a separator needs exactly as many dashes as the menu is wide.
 * Deriving the width from the dash string keeps the two from drifting.
 *
 * @param dashes a string literal of dashes.
 */
#define MV_MENU_WIDTH_OF(dashes) (sizeof(dashes) - 1)

/** @brief Default separator dash string; its length sets #MV_MENU_WIDTH. */
#define MV_MENU_DASHES           "-----------"   /* 11 dashes */
/** @brief Default dropdown width (the length of #MV_MENU_DASHES). */
#define MV_MENU_WIDTH            MV_MENU_WIDTH_OF(MV_MENU_DASHES)

/**
 * @brief One initially-enabled MIDSCR item row.
 *
 * Hides MIDSCR's trailing reserved bytes (always zero). Toggle enable/disable
 * at runtime with #mv_menu_item_set_enabled().
 *
 * @param title item label; at most 14 chars -- MIDSCR's name field is char[15]
 *              and needs room for the NUL, so cmoc rejects anything longer.
 */
#define MV_MENU_ITEM(title)          { (title), MN_ENBL, {0, 0, 0, 0, 0} }

/**
 * @brief One initially-disabled (greyed) MIDSCR item row.
 * @param title item label (see #MV_MENU_ITEM for the length limit).
 */
#define MV_MENU_ITEM_DISABLED(title) { (title), MN_DSBL, {0, 0, 0, 0, 0} }

/**
 * @brief A non-selectable separator row spanning the default menu width.
 * @see MV_MENU_SEPARATOR_S for a menu declared with a custom width.
 */
#define MV_MENU_SEPARATOR            MV_MENU_SEPARATOR_S(MV_MENU_DASHES)

/**
 * @brief A non-selectable separator row of a given width.
 * @param dashes a dash string whose length is the menu width -- so there is no
 *               per-width \#if ladder.
 */
#define MV_MENU_SEPARATOR_S(dashes)  { (dashes), MN_DSBL, {0, 0, 0, 0, 0} }

/**
 * @brief Enable or disable a menu item at runtime, hiding the @c _mienbl field
 *        and the MN_ENBL / MN_DSBL encoding.
 *
 * Intended for a refresh_menus_action callback.
 *
 * @param items a MIDSCR items[] array.
 * @param index index of the item within @p items.
 * @param on    any truthy value enables the item; falsy disables it.
 */
#define mv_menu_item_set_enabled(items, index, on) \
    ((items)[index]._mienbl = (char)((on) ? MN_ENBL : MN_DSBL))

/**
 * @brief One MNDSCR menu row of the default dropdown width.
 * @param title   menu-bar title.
 * @param menu_id menu id (e.g. MN_FILE).
 * @param items   the static MIDSCR items[] array for this menu.
 * @see MV_MENU_SIZED to set an explicit width.
 */
#define MV_MENU(title, menu_id, items) \
    MV_MENU_SIZED(title, menu_id, MV_MENU_WIDTH, items)

/**
 * @brief One MNDSCR menu row with an explicit dropdown width.
 *
 * To keep a custom-width menu's separators aligned, derive both the width and
 * the separators from one dash string:
 * @code
 * #define EDIT_DASHES "--------------"
 * static MIDSCR edit_items[] = {
 *     MV_MENU_ITEM("Undo"), MV_MENU_SEPARATOR_S(EDIT_DASHES), ...
 * };
 * ... MV_MENU_SIZED("Edit", MN_EDIT, MV_MENU_WIDTH_OF(EDIT_DASHES), edit_items) ...
 * @endcode
 *
 * @param title   menu-bar title.
 * @param menu_id menu id (e.g. MN_EDIT).
 * @param width   dropdown width (the byte cowin uses to size the dropdown).
 * @param items   the static MIDSCR items[] array for this menu.
 */
#define MV_MENU_SIZED(title, menu_id, width, items) \
    { (title), (menu_id), (width), sizeof(items) / sizeof((items)[0]), \
      MN_ENBL, {0, 0}, (items) }

/**
 * @brief Declare a static WNDSCR @p var with a menu bar, at the default minimum
 *        window size.
 * @param var   name of the static WNDSCR to declare.
 * @param title window title.
 * @param menus the static MNDSCR menus[] array.
 * @see mv_set_menus_sized to set an explicit minimum size.
 */
#define mv_set_menus(var, title, menus) \
    mv_set_menus_sized(var, title, menus, MV_WINDOW_MIN_WIDTH, MV_WINDOW_MIN_HEIGHT)

/**
 * @brief Declare a static WNDSCR @p var with a menu bar and an explicit minimum
 *        window size.
 *
 * A size below #MV_WINDOW_WIDTH_FLOOR x #MV_WINDOW_HEIGHT_FLOOR fails to
 * compile: the companion typedef then has a negative length (cmoc has no
 * @c _Static_assert).
 *
 * @param var    name of the static WNDSCR to declare.
 * @param title  window title.
 * @param menus  the static MNDSCR menus[] array.
 * @param width  minimum window width  (>= #MV_WINDOW_WIDTH_FLOOR).
 * @param height minimum window height (>= #MV_WINDOW_HEIGHT_FLOOR).
 */
#define mv_set_menus_sized(var, title, menus, width, height) \
    typedef char var##_mv_window_floor_ok[((width) >= MV_WINDOW_WIDTH_FLOOR && \
        (height) >= MV_WINDOW_HEIGHT_FLOOR) ? 1 : -1]; \
    static WNDSCR var = { \
        (title), \
        sizeof(menus) / sizeof((menus)[0]), \
        (width), (height), MV_WINDOW_SYNC, \
        {0, 0, 0, 0, 0, 0, 0}, \
        (menus) \
    }

/**
 * @brief Declare a static WNDSCR @p var with no menu bar, at the default
 *        minimum window size.
 * @param var   name of the static WNDSCR to declare.
 * @param title window title.
 * @see mv_menu_none_sized to set an explicit minimum size.
 */
#define mv_menu_none(var, title) \
    mv_menu_none_sized(var, title, MV_WINDOW_MIN_WIDTH, MV_WINDOW_MIN_HEIGHT)

/**
 * @brief Declare a static WNDSCR @p var with no menu bar and an explicit
 *        minimum window size (same floor check as #mv_set_menus_sized).
 * @param var    name of the static WNDSCR to declare.
 * @param title  window title.
 * @param width  minimum window width  (>= #MV_WINDOW_WIDTH_FLOOR).
 * @param height minimum window height (>= #MV_WINDOW_HEIGHT_FLOOR).
 */
#define mv_menu_none_sized(var, title, width, height) \
    typedef char var##_mv_window_floor_ok[((width) >= MV_WINDOW_WIDTH_FLOOR && \
        (height) >= MV_WINDOW_HEIGHT_FLOOR) ? 1 : -1]; \
    static WNDSCR var = { \
        (title), 0, \
        (width), (height), MV_WINDOW_SYNC, \
        {0, 0, 0, 0, 0, 0, 0}, \
        (MNDSCR *)0 \
    }

#endif /* _MVKIT_MV_MENU_H */
