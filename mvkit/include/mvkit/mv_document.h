#ifndef _MVKIT_MV_DOCUMENT_H
#define _MVKIT_MV_DOCUMENT_H

#include <mvkit/mv_defs.h>          /* error_code, MV_PATH_MAX */
#include <mvkit/mv_undo_manager.h>  /* MVUndoManager, MVUndoItem, bool */

/**
 * @file
 * @brief A document: data model + standard file lifecycle, loosely NSDocument.
 *
 * A document wraps an application's data model with the standard file lifecycle
 * (new / open / save / revert), dirty tracking, an undo manager, and the
 * save-before-discarding prompts.
 *
 * The app supplies an opaque @c model and three callbacks that act on it for a
 * given path; each returns 0 on success or an error code:
 *   - @c new_model  -- initialize a fresh, empty model
 *   - @c open_model -- load the model from a path
 *   - @c save_model -- write the model to a path
 *
 * Any callback may be NULL, which disables the corresponding operation (and the
 * matching mv_document_can_* query returns false).
 */

/**
 * @brief A document: an app's data model plus the standard file lifecycle,
 *        dirty tracking, and a per-document undo manager.
 */
typedef struct {
    void *model;                                      /**< app's data model (opaque to MVKit) */
    int (*new_model)(void *model, const char *path);  /**< create empty model; NULL disables New */
    int (*open_model)(void *model, const char *path); /**< load model from path; NULL disables Open */
    int (*save_model)(void *model, const char *path); /**< save model to path; NULL disables Save */
    int file_backed;            /**< nonzero once tied to a saved file on disk */
    const char *default_path;   /**< path used for a new, unsaved document */
    const char *extension;      /**< file extension incl. dot, e.g. ".xmt" (NULL for none) */
    char path[MV_PATH_MAX];     /**< current file path */
    MVUndoManager undo_manager; /**< per-document undo stack and dirty state */
} MVDocument;

/**
 * @brief Initialize @p doc with its model and callbacks.
 *
 * If @p path is non-NULL the document opens it (and becomes file-backed);
 * otherwise it starts as a new, unsaved document at @p default_path. The string
 * arguments are borrowed and must outlive @p doc.
 *
 * @param doc          the document to initialize.
 * @param path         file to open on init, or NULL to start a new document.
 * @param default_path path used for a new, unsaved document.
 * @param extension    file extension including its dot (e.g. ".xmt"), or NULL.
 * @param model        opaque app data model.
 * @param new_model    create-empty-model callback (NULL disables New).
 * @param open_model   load-model-from-path callback (NULL disables Open).
 * @param save_model   write-model-to-path callback (NULL disables Save).
 */
extern void mv_document_init(MVDocument *doc,
                          const char *path,
                          const char *default_path,
                          const char *extension,
                          void *model,
                          int (*new_model)(void *model, const char *path),
                          int (*open_model)(void *model, const char *path),
                          int (*save_model)(void *model, const char *path));

/**
 * @brief Start a new, empty document, prompting to save first if dirty.
 * @param doc the document.
 * @return true if a new document was created, false if declined or failed.
 */
extern bool mv_document_new(MVDocument *doc);

/**
 * @brief Prompt to save if dirty, show the Open dialog, and load the chosen file.
 * @param doc the document.
 * @return true if the open flow ran (a file was chosen, even if loading then
 *         errored), false if the user cancelled.
 */
extern bool mv_document_open(MVDocument *doc);

/**
 * @brief Mark @p doc as successfully opened from its current path.
 *
 * Sets it file-backed, clears undo state, and refreshes the menu bar. Use after
 * loading a document outside the Open dialog (e.g. a path given on the command
 * line).
 *
 * @param doc the document.
 */
extern void mv_document_opened(MVDocument *doc);

/**
 * @brief Reload the document from disk, discarding unsaved changes (prompting to
 *        save first if dirty).
 *
 * No-op if the document is not file-backed.
 *
 * @param doc the document.
 */
extern void mv_document_revert(MVDocument *doc);

/**
 * @brief Show the Save dialog and write the document to the chosen path.
 * @param doc the document.
 * @return 0 on success (or if cancelled), or an error code from the save callback.
 */
extern error_code mv_document_save_as(MVDocument *doc);

/**
 * @brief Save to the current path, or fall back to mv_document_save_as() if the
 *        document is not yet file-backed.
 * @param doc the document.
 * @return 0 on success, or an error code.
 */
extern error_code mv_document_save(MVDocument *doc);

/**
 * @brief Record a model change and its corresponding undo action.
 *
 * The action is pushed onto the document's undo manager and the document is
 * marked dirty.
 *
 * @param doc       the document.
 * @param undo_item the undo action for this change (copied by value).
 */
extern void mv_document_make_change(MVDocument *doc, const MVUndoItem *undo_item);

/**
 * @brief Whether the document has unsaved changes.
 * @param doc the document.
 * @return true if dirty.
 */
extern bool mv_document_is_dirty(const MVDocument *doc);

/**
 * @brief Whether New is available (a new_model callback was supplied).
 * @param doc the document.
 * @return true if the operation is available.
 */
extern bool mv_document_can_new(const MVDocument *doc);
/**
 * @brief Whether Open is available (an open_model callback was supplied).
 * @param doc the document.
 * @return true if the operation is available.
 */
extern bool mv_document_can_open(const MVDocument *doc);
/**
 * @brief Whether Revert is available (file-backed with a save/open callback).
 * @param doc the document.
 * @return true if the operation is available.
 */
extern bool mv_document_can_revert(const MVDocument *doc);
/**
 * @brief Whether Save is available (a save_model callback was supplied).
 * @param doc the document.
 * @return true if the operation is available.
 */
extern bool mv_document_can_save(const MVDocument *doc);
/**
 * @brief Whether there is a change available to undo.
 * @param doc the document.
 * @return true if an action can be undone.
 */
extern bool mv_document_can_undo(const MVDocument *doc);

/**
 * @brief Undo the most recent change.
 * @param doc the document.
 * @return true if an action was undone.
 */
extern bool mv_document_undo(MVDocument *doc);

#endif /* _MVKIT_MV_DOCUMENT_H */
