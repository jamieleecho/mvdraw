#include <stdlib.h>   /* NULL */

#include "mvkit/mv_document.h"

/* Tiny capability/state queries, grouped because they are trivial and commonly
   used together (e.g. when refreshing menu-item enable state). */


bool mv_document_is_dirty(const MVDocument *doc) {
    /* Possible optimizer error when we use ! instead of == */
    return mv_undo_manager_all_undone(&(doc->undo_manager)) == 0;
}


bool mv_document_can_new(const MVDocument *doc) {
    return doc->new_model != NULL;
}


bool mv_document_can_open(const MVDocument *doc) {
    return doc->open_model != NULL;
}


bool mv_document_can_revert(const MVDocument *doc) {
    return doc->open_model != NULL;
}


bool mv_document_can_save(const MVDocument *doc) {
    return doc->save_model != NULL;
}


bool mv_document_can_undo(const MVDocument *doc) {
    return mv_undo_manager_can_undo(&(doc->undo_manager));
}
