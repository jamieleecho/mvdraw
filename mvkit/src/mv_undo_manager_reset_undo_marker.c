#include "mvkit/mv_undo_manager.h"

void mv_undo_manager_reset_undo_marker(MVUndoManager *undo_manager) {
    undo_manager->undo_marker = 0;
}
