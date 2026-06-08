#include <string.h>

#include "mv_document_internal.h"


void mv_document_init(MVDocument *doc,
                   const char *path,
                   const char *default_path,
                   const char *extension,
                   void *model,
                   int (*new_model)(void *model, const char *path),
                   int (*open_model)(void *model, const char *path),
                   int (*save_model)(void *model, const char *path)) {
    doc->model = model;
    doc->new_model = new_model;
    doc->save_model = save_model;
    doc->open_model = open_model;
    doc->file_backed = false;
    doc->default_path = default_path;
    doc->extension = extension;
    strncpy(doc->path, path ? path : default_path, sizeof(doc->path));
    doc->path[MV_PATH_MAX - 1] = 0;
    if (path) {
        mv_document_revert(doc);
    }
    mv_document_ensure_extension(doc);
    mv_undo_manager_init(&(doc->undo_manager));
}
