#include <mvkit/mv_app.h>
#include "mvkit/mv_document.h"


void mv_document_make_change(MVDocument *doc, const MVUndoItem *undo_item) {
    if (mv_undo_manager_all_undone(&(doc->undo_manager))) {
        mv_app_refresh_menubar();
    }
    mv_undo_manager_push_undo(&(doc->undo_manager), undo_item);
}
