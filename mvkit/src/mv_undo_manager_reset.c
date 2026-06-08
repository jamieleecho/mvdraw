#include "mvkit/mv_undo_manager.h"

void mv_undo_manager_reset(MVUndoManager *undo_manager) {
    undo_manager->index = 0;
    undo_manager->undo_marker = 0;
}
