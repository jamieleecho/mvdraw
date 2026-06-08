#include <mvkit/mv_app.h>
#include "mvkit/mv_document.h"


bool mv_document_undo(MVDocument *doc) {
    if (mv_undo_manager_all_undone(&(doc->undo_manager))) {
        return false;
    }
    int val = mv_undo_manager_undo(&(doc->undo_manager));
    if (val || mv_undo_manager_all_undone(&(doc->undo_manager))) {
        mv_app_refresh_menubar();
    }
    return val;
}
