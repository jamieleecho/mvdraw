#include <cgfx.h>

#include "mvkit/mv_defs.h"    /* MV_OUTPATH */
#include "mvkit/mv_theme.h"

void mv_app_set_theme(const MVTheme *theme) {
    int ii;
    for (ii = 0; ii < 16; ++ii) {
        _cgfx_palette(MV_OUTPATH, ii, theme->raw[ii]);
    }
}
