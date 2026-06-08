#include <string.h>

#include "mv_document_internal.h"


char mv_document_message[128];


/* Derived from Google AI */
static int str_end_cmp(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) {
        return 1;
    }
    return strcmp(str + str_len - suffix_len, suffix);
}


void mv_document_ensure_extension(MVDocument *doc) {
    if (str_end_cmp(doc->path, doc->extension)) {
        size_t str_len = strlen(doc->path);
        size_t suffix_len = strlen(doc->extension);
        if ((str_len + suffix_len) < sizeof(doc->path)) {
            strncat(doc->path + str_len, doc->extension, sizeof(doc->path) - 1 - suffix_len);
        }
    }
}
