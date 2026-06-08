#include <mvkit/mv_app.h>
#include "mv_document_internal.h"


error_code mv_document_save_as(MVDocument *doc) {
    if (!doc->save_model) {
        return 0;
    }

    if (!mv_app_show_save_dialog(doc->path, doc->extension)) {
        return 0;
    }

    return mv_document_save_internal(doc, false);
}
