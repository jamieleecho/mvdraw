#include "mvkit/mv_undo_manager.h"

bool mv_undo_manager_can_undo(const MVUndoManager *undo_manager) {
    return undo_manager->index > 0;
}
