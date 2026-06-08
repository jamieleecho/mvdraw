#include <mvkit/mv_app.h>
#include "mvkit/mv_document.h"


void mv_document_opened(MVDocument *doc) {
    doc->file_backed = true;
    mv_undo_manager_reset(&(doc->undo_manager));
    mv_app_refresh_menubar();
}
