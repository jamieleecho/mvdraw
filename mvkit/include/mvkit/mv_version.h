#ifndef _MVKIT_MV_VERSION_H
#define _MVKIT_MV_VERSION_H

/**
 * @file
 * @brief MVKit framework version.
 */

/** @brief MVKit major version number. */
#define MVKIT_VERSION_MAJOR 0
/** @brief MVKit minor version number. */
#define MVKIT_VERSION_MINOR 1
/** @brief MVKit patch version number. */
#define MVKIT_VERSION_PATCH 0

/**
 * @brief The MVKit framework version.
 * @return a static "MAJOR.MINOR.PATCH" string (do not free).
 */
extern const char *mv_version(void);

#endif /* _MVKIT_MV_VERSION_H */
