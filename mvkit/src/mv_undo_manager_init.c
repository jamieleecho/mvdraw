#include "mvkit/mv_undo_manager.h"

void mv_undo_manager_init(MVUndoManager *undo_manager) {
    mv_undo_manager_reset(undo_manager);
}
