#include "mv_document_internal.h"


error_code mv_document_save(MVDocument *doc) {
    if (!doc->save_model) {
        return 0;
    }

    if (!doc->file_backed) {
        return mv_document_save_as(doc);
    } else {
        return mv_document_save_internal(doc, true);
    }
}
