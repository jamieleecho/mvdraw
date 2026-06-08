#include "mvkit/mv_undo_manager.h"

bool mv_undo_manager_undo(MVUndoManager *undo_manager) {
    if (!mv_undo_manager_can_undo(undo_manager)) {
        return false;
    }
    MVUndoItem undo_item = undo_manager->undo_items[--undo_manager->index];
    undo_item.undo_function(undo_item.object);
    undo_manager->undo_marker = undo_manager->undo_marker - 1;
    return true;
}
