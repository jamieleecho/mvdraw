#include <stdio.h>
#include <string.h>

#include <mvkit/mv_app.h>
#include "mv_document_internal.h"


bool mv_document_new(MVDocument *doc) {
    if (!doc->new_model) {
        return false;
    }

    if (mv_document_is_dirty(doc)) {
        if (mv_app_show_message_box("Save before starting\r\na new document?", MVMessageBoxType_YesNo) ==
             MVMessageBoxResult_Yes) {
            if (mv_document_save(doc)) {
                mv_app_show_message_box("New aborted.", MVMessageBoxType_Info);
                return false;
            }
        }
    }

    int err = doc->new_model(doc->model, doc->path);

    if (err) {
        sprintf(mv_document_message, "Failed to create\r\ndocument.\r\nError = %d", err);
        mv_app_show_message_box(mv_document_message, MVMessageBoxType_Error);
        return false;
    }
    strncpy(doc->path, doc->default_path, sizeof(doc->path));
    doc->path[MV_PATH_MAX - 1] = 0;
    mv_document_ensure_extension(doc);
    doc->file_backed = false;
    mv_undo_manager_reset(&(doc->undo_manager));
    mv_app_refresh_menubar();
    return true;
}
