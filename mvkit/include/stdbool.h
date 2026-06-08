#ifndef _STDBOOL_H
#define _STDBOOL_H

/**
 * @file
 * @brief Minimal `<stdbool.h>` shim for cmoc.
 *
 * cmoc has no `<stdbool.h>`, so MVKit ships this so the framework is
 * self-contained. The include guard matches the app's own stdbool.h so the two
 * never produce a duplicate `bool` typedef in one translation unit.
 */

/** @brief Boolean type: #false or #true. */
typedef enum {
    false = 0,   /**< boolean false */
    true = 1     /**< boolean true */
} bool;

#endif /* _STDBOOL_H */
