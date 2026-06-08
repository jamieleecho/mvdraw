#ifndef _MVKIT_MV_FILE_DIALOG_H
#define _MVKIT_MV_FILE_DIALOG_H

#include <cgfx.h>   /* path_id */

/**
 * @file
 * @brief MVKit's Save/Open file browser.
 *
 * Cloned from cmoc_os9's cgfx MVFName() and customized with confirm + Cancel
 * buttons.
 */

/**
 * @brief Show a modal file browser and return the chosen filename.
 *
 * A Cancel button is always shown. When @p allow_new is set, a "[new file]"
 * entry prompts for a name (use it for Save, not Open) and the chosen name
 * gets @p ext appended if missing. When @p ext is set, the listing shows only
 * matching files plus directories.
 *
 * @param path          open cgfx path for drawing.
 * @param title         dialog title.
 * @param confirm_label text on the confirm button (e.g. "Open" or "Save").
 * @param allow_new     nonzero to offer a "[new file]" entry (Save behaviour).
 * @param ext           extension including its leading dot (e.g. ".xmt"), or
 *                      NULL for none / list all files.
 * @param column        column of the dialog's top-left corner.
 * @param row           row of the dialog's top-left corner.
 * @param fg            foreground palette register.
 * @param bg            background palette register.
 * @return a pointer to the chosen filename, or NULL if cancelled.
 */
char *mv_file_dialog(path_id path, const char *title, const char *confirm_label,
                     int allow_new, const char *ext,
                     int column, int row, int fg, int bg);

#endif /* _MVKIT_MV_FILE_DIALOG_H */
