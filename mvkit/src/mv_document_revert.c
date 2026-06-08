#include <string.h>

#include <mvkit/mv_app.h>
#include "mv_document_internal.h"


void mv_document_revert(MVDocument *doc) {
    if (!doc->open_model || !doc->file_backed) {
        return;
    }

    if (mv_document_is_dirty(doc)) {
        char oldpath[MV_PATH_MAX];
        strncpy(oldpath, doc->path, sizeof(oldpath));
        oldpath[MV_PATH_MAX - 1] = 0;
        if (mv_app_show_message_box("Save before reverting\r\ndocument?", MVMessageBoxType_YesNo) ==
             MVMessageBoxResult_Yes) {
            if (mv_document_save_internal(doc, true)) {
                strcpy(doc->path, oldpath);
                mv_app_show_message_box("Revert aborted.", MVMessageBoxType_Info);
                return;
            }
        }
        strcpy(doc->path, oldpath);
        mv_undo_manager_reset(&(doc->undo_manager));
        mv_app_refresh_menubar();
    }
}
