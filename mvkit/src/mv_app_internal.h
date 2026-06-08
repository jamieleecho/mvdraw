#ifndef _MVKIT_MV_APP_INTERNAL_H
#define _MVKIT_MV_APP_INTERNAL_H

/* Foreground/background colors for MVKit's built-in dialogs, shared by the
   message-box and file-dialog translation units. Sourced from the theme's
   dialog role so MVKit's own dialogs stay consistent with the chrome. */
#include <mvkit/mv_theme.h>

#define MV_APP_FOREGROUND_COLOR MV_THEME_DIALOG_FG
#define MV_APP_BACKGROUND_COLOR MV_THEME_DIALOG_BG

#endif /* _MVKIT_MV_APP_INTERNAL_H */
