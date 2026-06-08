#ifndef _MVKIT_MV_DEFS_H
#define _MVKIT_MV_DEFS_H

#include <os.h>   /* error_code */

/**
 * @file
 * @brief Shared MVKit constants and the canonical Flush() declaration.
 */

/** @brief OS-9 standard input path number, used for cgfx input. */
#define MV_INPATH  0
/** @brief OS-9 standard output path number, used for all cgfx drawing. */
#define MV_OUTPATH 1

/** @brief Maximum length, including the NUL, of an MVKit file-path buffer. */
#define MV_PATH_MAX 44

/**
 * @brief Flush buffered graphics output to the screen.
 *
 * Implemented in libcgfx (cbuffer.as, as the exported @c _Flush symbol);
 * declared here so MVKit modules and apps need not each forward-declare it.
 *
 * @return 0 on success, or an OS-9 error code.
 */
error_code Flush(void);

#endif /* _MVKIT_MV_DEFS_H */
