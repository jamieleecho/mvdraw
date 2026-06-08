#ifndef _MVKIT_MV_THEME_H
#define _MVKIT_MV_THEME_H

#include <os.h>          /* byte */
#include <stdbool.h>

/**
 * @file
 * @brief MVTheme -- the 16-entry color palette an MVKit app installs at startup.
 *
 * The OS-9 / NitrOS-9 window manager (cowin) draws ALL window chrome from a
 * hardcoded color table tied to palette registers 0-3: the menu bar, dropdown
 * menus, the WT_DBOX dialog double-border, shadows, scrollbars and the 3D
 * edges. You cannot change WHICH registers it uses -- only their RGB. The
 * window manager's own rule:
 *
 *     Registers 0-3 are the window-chrome ramp and MUST be ordered
 *     darkest -> lightest. Registers 4-15 are the app's to use freely.
 *
 * MVTheme makes that hard to get wrong by NAMING the four chrome slots, so you
 * can't accidentally treat register 1 as "bright white".
 */

/**
 * @brief A theme: the 16 palette registers, addressable as a flat array or by
 *        named chrome slot.
 *
 * The union lets you load the registers as a flat array (@c raw, used by
 * mv_app_set_theme()) or address the meaningful slots by name (@c ramp.*). cmoc
 * has no C11 anonymous union members, so the named slots live under @c ramp.
 * The union is exactly 16 bytes, so @c raw[i] aliases the matching ramp field.
 * Values are cgfx color numbers (colno).
 */
typedef union {
    byte raw[16];                /**< all 16 registers, flat */
    struct {
        byte chrome_darkest;     /**< reg 0 -- e.g. black; window/content base */
        byte chrome_dark;        /**< reg 1 -- e.g. dark grey; shadows, dialog border */
        byte chrome_light;       /**< reg 2 -- e.g. light grey; menu bar, bars */
        byte chrome_lightest;    /**< reg 3 -- e.g. white; 3D highlights, text */
        byte content[12];        /**< regs 4-15 -- the app's to define freely */
    } ramp;                      /**< named registers (chrome ramp 0-3 + content 4-15) */
} MVTheme;

/** @brief Register MVKit uses for message-box / dialog interior text. */
#define MV_THEME_DIALOG_FG     0
/** @brief Register MVKit uses for message-box / dialog interior fill. */
#define MV_THEME_DIALOG_BG     3
/** @brief Register MVKit uses for an MVImageGrid border / highlight. */
#define MV_THEME_CONTROL_FG    3
/** @brief Register MVKit uses for an MVImageGrid background. */
#define MV_THEME_CONTROL_BG    0
/** @brief Register MVKit uses for the frame border (_cgfx_border). */
#define MV_THEME_WINDOW_BORDER 0

/**
 * @brief The canonical Multi-Vue look.
 *
 * Black / dark-grey / light-grey / white ramp in registers 0-3, over a
 * general-purpose content spread in 4-15. Apps usually define their own theme
 * (with their own @c content) and keep the four chrome values; this is the
 * standard look and a usable fallback.
 */
extern const MVTheme mv_theme_default;

/**
 * @brief Install a theme: load all 16 palette registers (chrome 0-3 + content
 *        4-15) via _cgfx_palette.
 *
 * Call in pre_init, before mv_app_run sets up the window. The theme-aware,
 * higher-level form of mv_app_init().
 *
 * @param theme the theme to install.
 */
extern void mv_app_set_theme(const MVTheme *theme);

/**
 * @brief Whether the chrome ramp is non-decreasing in brightness (darkest ->
 *        lightest).
 *
 * A development-time check -- a theme that fails it makes the 3D chrome look
 * wrong. Not meant to run in release builds (RAM/cycles are scarce); guard a
 * call to it behind a debug build.
 *
 * @param theme the theme to check.
 * @return true if registers 0-3 run darkest to lightest.
 */
extern bool mv_theme_chrome_ordered(const MVTheme *theme);

#endif /* _MVKIT_MV_THEME_H */
