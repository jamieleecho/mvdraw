#include <cgfx.h>
#include <string.h>

#include "mvkit/mv_app.h"
#include "mvkit/mv_file_dialog.h"
#include "mv_app_internal.h"


/* mv_file_dialog()'s box footprint (incl. WT_DBOX border), for centering. */
#define FILEDIALOG_WIDTH  24
#define FILEDIALOG_HEIGHT 14

/* Center mv_file_dialog() on screen and copy the chosen name into `path`.
   Returns `path` on confirm, NULL on cancel. The browser starts in the CWD;
   `path` is an output buffer only. `allow_new` adds the "[new file]" entry.
   `ext` is the document extension including its dot (e.g. ".xmt"), or NULL.
   On confirm the CWD is left at the chosen file's directory; on cancel the
   dialog restores the starting CWD. */
static char *mv_file_dialog_centered(const char *title, const char *confirm_label,
                                     int allow_new, const char *ext, char *path) {
    int sx, sy;
    char *result;

    if (_cgfx_gs_scsz(MV_OUTPATH, &sx, &sy)) {
        sx = 0;
        sy = 0;
    } else {
        sx = (sx - FILEDIALOG_WIDTH) / 2;
        sy = (sy - FILEDIALOG_HEIGHT) / 2;
        if (sx < 0) {
            sx = 0;
        }
        if (sy < 0) {
            sy = 0;
        }
    }

    result = mv_file_dialog(MV_OUTPATH, title, confirm_label, allow_new, ext,
                            sx, sy, MV_APP_FOREGROUND_COLOR, MV_APP_BACKGROUND_COLOR);

    if (result) {
        strncpy(path, result, MV_PATH_MAX - 1);
        path[MV_PATH_MAX - 1] = 0;
        return path;
    }
    return (char *)NULL;
}

char *mv_app_show_open_dialog(char *path, const char *ext) {
    return mv_file_dialog_centered("Open File", "Open", 0, ext, path);
}


char *mv_app_show_save_dialog(char *path, const char *ext) {
    return mv_file_dialog_centered("Save File", "Save", 1, ext, path);
}
