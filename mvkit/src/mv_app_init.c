#include <cgfx.h>
#include <stdlib.h>   /* size_t */

#include "mvkit/mv_app.h"


void mv_app_init(const int *palette, size_t num_colors) {
    for (size_t ii = 0; ii < num_colors; ++ii) {
        _cgfx_palette(MV_OUTPATH, ii, palette[ii]);
    }
}
