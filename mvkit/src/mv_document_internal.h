#ifndef _MVKIT_MV_DOCUMENT_INTERNAL_H
#define _MVKIT_MV_DOCUMENT_INTERNAL_H

/* Private interface shared among the mv_document_*.c translation units. Not
   part of the public API (lives in src/, not include/). */

#include "mvkit/mv_document.h"

/* Single shared scratch buffer for formatting error messages. One instance
   keeps stack use low on the 6809; uses never temporally overlap (e.g. the
   new -> save -> save_internal chain writes it only after the inner call
   returns). */
extern char mv_document_message[128];

/* Append doc->extension to doc->path if the path does not already end with it
   (and it fits). */
void mv_document_ensure_extension(MVDocument *doc);

/* Shared save core behind mv_document_save / _save_as / _revert. When
   force_overwrite is false, prompts before clobbering an existing file.
   Returns 0 on success (or decline) or the save callback's error code. */
error_code mv_document_save_internal(MVDocument *doc, bool force_overwrite);

#endif /* _MVKIT_MV_DOCUMENT_INTERNAL_H */
