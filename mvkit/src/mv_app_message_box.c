#include <cgfx.h>
#include <string.h>

#include "mvkit/mv_app.h"
#include "mv_app_internal.h"


static int mv_dialog_centered(DIALOG *dialog, int width, int height) {
    int sx, sy, result;

    if (_cgfx_gs_scsz(MV_OUTPATH, &sx, &sy)) {
        sx = 0;
        sy = 0;
    } else {
        sx = (sx - width) / 2;
        sy = (sy - height) / 2;
        if (sx < 0) {
            sx = 0;
        }
        if (sy < 0) {
            sy = 0;
        }
    }

    result = Dialog(MV_OUTPATH, dialog, sx, sy, width, height,
                    MV_APP_FOREGROUND_COLOR, MV_APP_BACKGROUND_COLOR);
    return result;
}


/* Message box drawn with cmoc_os9's button widget via Dialog(). The frame
   costs one character on each edge, so usable columns are 1..(WIDTH-2). */
#define MSGBOX_WIDTH      28
#define MSGBOX_HEIGHT     10
#define MSGBOX_INTERIOR   (MSGBOX_WIDTH - 2)
#define MSGBOX_TEXT_ROW   2
#define MSGBOX_BUTTON_ROW 6
#define MSGBOX_MAX_LINES  3

#define MSGBOX_VAL_PRIMARY   1  /* OK / Yes */
#define MSGBOX_VAL_SECONDARY 2  /* Cancel / No */

static int add_button(DIALOG *item, int column, int row, int key, int val, char *label) {
    item->d_type = D_BUTTON;
    item->d_column = (char)column;
    item->d_row = (char)row;
    item->d_key = (char)key;
    item->d_val = (char)val;
    item->d_string = label;
    return 1;
}

MVMessageBoxResult mv_app_show_message_box(const char *message, MVMessageBoxType event_type) {
    char buf[80];
    char *lines[MSGBOX_MAX_LINES];
    DIALOG items[MSGBOX_MAX_LINES + 3];  /* lines + up to 2 buttons + D_END */
    int num_lines = 0;
    int n = 0;
    int val;
    char *p;

    /* Split the message into up to MSGBOX_MAX_LINES lines on CR/LF. */
    strncpy(buf, message, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = 0;
    p = buf;
    lines[num_lines++] = p;
    while (*p && num_lines < MSGBOX_MAX_LINES) {
        if (*p == '\r' || *p == '\n') {
            *p++ = 0;
            while (*p == '\r' || *p == '\n') {
                p++;
            }
            if (!*p) {
                break;
            }
            lines[num_lines++] = p;
        } else {
            p++;
        }
    }

    /* Text lines, centered horizontally within the interior. */
    for (int ii = 0; ii < num_lines; ++ii) {
        int len = strlen(lines[ii]);
        items[n].d_type = D_TEXT;
        items[n].d_column = (char)(1 + (MSGBOX_INTERIOR - len) / 2);
        items[n].d_row = (char)(MSGBOX_TEXT_ROW + ii);
        items[n].d_key = 0;
        items[n].d_val = 0;
        items[n].d_string = lines[ii];
        n++;
    }

    /* Buttons, centered as a row, depending on the box type. */
    if (event_type == MVMessageBoxType_YesNo) {
        n += add_button(&items[n],  8, MSGBOX_BUTTON_ROW, '\r', MSGBOX_VAL_PRIMARY,   (char *)" Yes ");
        n += add_button(&items[n], 15, MSGBOX_BUTTON_ROW, 0x1b, MSGBOX_VAL_SECONDARY, (char *)" No ");
    } else if (event_type == MVMessageBoxType_OkCancel) {
        n += add_button(&items[n],  8, MSGBOX_BUTTON_ROW, '\r', MSGBOX_VAL_PRIMARY,   (char *)" OK ");
        n += add_button(&items[n], 14, MSGBOX_BUTTON_ROW, 0x1b, MSGBOX_VAL_SECONDARY, (char *)"Cancel");
    } else {
        n += add_button(&items[n], 11, MSGBOX_BUTTON_ROW, '\r', MSGBOX_VAL_PRIMARY,   (char *)"  OK  ");
    }

    items[n].d_type = D_END;
    items[n].d_column = 0;
    items[n].d_row = 0;
    items[n].d_key = 0;
    items[n].d_val = 0;
    items[n].d_string = (char *)NULL;

    val = mv_dialog_centered(items, MSGBOX_WIDTH, MSGBOX_HEIGHT);

    switch (event_type) {
        case MVMessageBoxType_YesNo:
            return (val == MSGBOX_VAL_PRIMARY) ? MVMessageBoxResult_Yes : MVMessageBoxResult_No;
        case MVMessageBoxType_OkCancel:
            return (val == MSGBOX_VAL_PRIMARY) ? MVMessageBoxResult_Ok : MVMessageBoxResult_Cancel;
        default:
            return MVMessageBoxResult_Ok;
    }
}
