#ifndef _MVKIT_MV_UNDO_MANAGER_H
#define _MVKIT_MV_UNDO_MANAGER_H

#include <stdbool.h>

/**
 * @file
 * @brief A bounded undo stack with dirty tracking.
 */

/** @brief Capacity of the undo stack. Pushing past this drops the oldest action. */
#define MV_UNDO_MANAGER_MAX_UNDOS 16

/**
 * @brief A single reversible action.
 *
 * mv_undo_manager_undo() calls @c undo_function(object) to roll it back. The
 * caller owns whatever @c object points to and must keep it valid until the
 * action is undone or discarded.
 */
typedef struct {
    void (*undo_function)(void *object);   /**< rolls the action back */
    void *object;                          /**< caller-owned argument to undo_function */
} MVUndoItem;

/**
 * @brief A bounded stack of #MVUndoItem plus a "dirty" counter.
 *
 * @c index is the stack depth (how many actions can still be undone).
 * @c undo_marker counts net changes since the last save baseline: push_undo
 * increments it, undo decrements it, and it reads zero exactly when the document
 * is back at its last saved/initialized state (see mv_undo_manager_all_undone()).
 * Treat the fields as private; use the functions below.
 */
typedef struct {
    MVUndoItem undo_items[MV_UNDO_MANAGER_MAX_UNDOS];   /**< the action ring */
    int index;         /**< current stack depth */
    int undo_marker;   /**< net changes since the clean baseline */
} MVUndoManager;

/**
 * @brief Initialize @p undo_manager to an empty, clean state. Call once before use.
 * @param undo_manager the manager to initialize.
 */
extern void mv_undo_manager_init(MVUndoManager *undo_manager);

/**
 * @brief Push @p item onto the undo stack (copied by value) and mark the
 *        document changed.
 *
 * If the stack is full, the oldest action is discarded to make room.
 *
 * @param undo_manager the undo manager.
 * @param item         the action to push (copied by value).
 */
extern void mv_undo_manager_push_undo(MVUndoManager *undo_manager, const MVUndoItem *item);

/**
 * @brief Whether there is at least one action available to undo.
 * @param undo_manager the undo manager.
 * @return true if an action can be undone.
 */
extern bool mv_undo_manager_can_undo(const MVUndoManager *undo_manager);

/**
 * @brief Undo the most recent action: pop it and invoke its undo_function.
 * @param undo_manager the undo manager.
 * @return true if an action was undone, false if the stack was empty.
 */
extern bool mv_undo_manager_undo(MVUndoManager *undo_manager);

/**
 * @brief Whether the document is at its saved/initialized baseline (no net
 *        unsaved changes).
 *
 * The inverse is what mv_document_is_dirty() reports.
 *
 * @param undo_manager the undo manager.
 * @return true if there are no net unsaved changes.
 */
extern bool mv_undo_manager_all_undone(const MVUndoManager *undo_manager);

/**
 * @brief Mark the current state as the clean baseline (e.g. just after a save)
 *        WITHOUT clearing the undo stack, so prior actions remain undoable.
 * @param undo_manager the undo manager.
 */
extern void mv_undo_manager_reset_undo_marker(MVUndoManager *undo_manager);

/**
 * @brief Clear the undo stack and the dirty counter (e.g. on new/open).
 * @param undo_manager the undo manager.
 */
extern void mv_undo_manager_reset(MVUndoManager *undo_manager);

#endif /* _MVKIT_MV_UNDO_MANAGER_H */
