#ifndef _MVKIT_MV_EVENT_H
#define _MVKIT_MV_EVENT_H

#include <cgfx.h>   /* MSRET */

/**
 * @file
 * @brief Input events delivered to views and the application.
 *
 * Kept in its own foundational header so lower-level pieces (e.g. mv_view) can
 * use events without depending on the application module.
 */

/** @brief Which kind of input an #MVUiEvent carries. */
typedef enum {
    MVUiEventType_KeyPress,    /**< a key was pressed (see #MVKeyEvent) */
    MVUiEventType_MouseClick   /**< the mouse was clicked (see the cgfx MSRET) */
} MVUiEventType;

/** @brief Payload of a key-press event: the character read from the keyboard. */
typedef struct {
    char character;   /**< the key character */
} MVKeyEvent;

/**
 * @brief A UI event delivered to a view or the application action callback.
 *
 * @c event_type selects the active member of @c info: a #MVKeyEvent for key
 * presses, or the cgfx MSRET mouse record for clicks.
 */
typedef struct {
    MVUiEventType event_type;   /**< which member of @c info is valid */
    union {
        MVKeyEvent key;    /**< valid when event_type == MVUiEventType_KeyPress */
        MSRET mouse;       /**< valid when event_type == MVUiEventType_MouseClick */
    } info;   /**< the event payload, selected by @c event_type */
} MVUiEvent;

#endif /* _MVKIT_MV_EVENT_H */
