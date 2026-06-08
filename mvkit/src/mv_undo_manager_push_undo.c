#include <string.h>

#include "mvkit/mv_undo_manager.h"

void mv_undo_manager_push_undo(MVUndoManager *undo_manager, const MVUndoItem *item) {
    if (undo_manager->index >= (MV_UNDO_MANAGER_MAX_UNDOS - 1)) {
        memcpy((char *)undo_manager->undo_items, (char *)(undo_manager->undo_items + 1), sizeof(undo_manager->undo_items) - sizeof(undo_manager->undo_items[0]));
        undo_manager->undo_items[MV_UNDO_MANAGER_MAX_UNDOS - 1] = *item;
    } else {
        undo_manager->undo_items[undo_manager->index++] = *item;
    }
    undo_manager->undo_marker = undo_manager->undo_marker + 1;
}
