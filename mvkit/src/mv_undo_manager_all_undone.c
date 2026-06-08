#include "mvkit/mv_undo_manager.h"

bool mv_undo_manager_all_undone(const MVUndoManager *undo_manager) {
    return undo_manager->undo_marker == 0;
}
