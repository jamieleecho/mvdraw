#include <stdio.h>

#include <mvkit/mv_app.h>
#include "mv_document_internal.h"


bool mv_document_open(MVDocument *doc) {
    if (!doc->open_model) {
        return false;
    }

    if (mv_document_is_dirty(doc)) {
        if (mv_app_show_message_box("Save before opening\r\na new document?", MVMessageBoxType_YesNo) ==
             MVMessageBoxResult_Yes) {
            if (mv_document_save(doc)) {
                mv_app_show_message_box("Open aborted.", MVMessageBoxType_Info);
                return false;
            }
        }
    }

    if (!mv_app_show_open_dialog(doc->path, doc->extension)) {
        return false;
    }

    mv_document_ensure_extension(doc);
    _cgfx_setgc(MV_OUTPATH, GRP_PTR, PTR_SLP);
    Flush();
    int err = doc->open_model(doc->model, doc->path);
    _cgfx_setgc(MV_OUTPATH, GRP_PTR, PTR_ARR);
    Flush();
    if (err) {
        sprintf(mv_document_message, "Failed to load document.\r\nError = %d", err);
        mv_app_show_message_box(mv_document_message, MVMessageBoxType_Error);
        doc->file_backed = false;
        return true;
    }
    mv_document_opened(doc);
    return true;
}
