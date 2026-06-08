#include "mvkit/mv_theme.h"

/* Approximate brightness of a CoCo3 color number. A colno is %R1 G1 B1 R0 G0 B0;
   each channel is a 2-bit value (high bit + low bit), 0..3. Summing the three
   channels gives 0..9 -- coarse, but enough to order the chrome ramp. */
static byte mv_theme_brightness(byte colno) {
    byte r = (byte)(((colno >> 5) & 1) * 2 + ((colno >> 2) & 1));
    byte g = (byte)(((colno >> 4) & 1) * 2 + ((colno >> 1) & 1));
    byte b = (byte)(((colno >> 3) & 1) * 2 + ((colno >> 0) & 1));
    return (byte)(r + g + b);
}

bool mv_theme_chrome_ordered(const MVTheme *theme) {
    byte b0 = mv_theme_brightness(theme->ramp.chrome_darkest);
    byte b1 = mv_theme_brightness(theme->ramp.chrome_dark);
    byte b2 = mv_theme_brightness(theme->ramp.chrome_light);
    byte b3 = mv_theme_brightness(theme->ramp.chrome_lightest);
    return b0 <= b1 && b1 <= b2 && b2 <= b3;
}
