#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <mvkit/mv_app.h>
#include "mv_document_internal.h"


error_code mv_document_save_internal(MVDocument *doc, bool force_overwrite) {
    mv_document_ensure_extension(doc);

    if (!force_overwrite) {
        int fd = open(doc->path, FAP_READ);
        if (fd >= 0) {
            close(fd);
            if (mv_app_show_message_box("Overwrite existing\r\nfile?", MVMessageBoxType_YesNo) == MVMessageBoxResult_No) {
                return 0;
            }
        }
    }

    _cgfx_setgc(MV_OUTPATH, GRP_PTR, PTR_SLP);
    Flush();
    int err = doc->save_model(doc->model, doc->path);
    _cgfx_setgc(MV_OUTPATH, GRP_PTR, PTR_ARR);
    Flush();
    if (err) {
        sprintf(mv_document_message, "Failed to save document.\r\nError = %d", err);
        mv_app_show_message_box(mv_document_message, MVMessageBoxType_Error);
        return err;
    }
    doc->file_backed = true;
    mv_undo_manager_reset_undo_marker(&(doc->undo_manager));
    mv_app_refresh_menubar();
    return 0;
}
