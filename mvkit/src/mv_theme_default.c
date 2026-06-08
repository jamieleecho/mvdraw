#include "mvkit/mv_theme.h"

/* The canonical Multi-Vue look. cmoc has no designated initializers, so this is
   a positional initializer of the union's first member (raw[16]); the ramp
   fields alias it. Registers 0-3 are the standard chrome ramp (darkest ->
   lightest); 4-15 are a general-purpose content spread that apps typically
   replace with their own colors. Values are cgfx color numbers (colno,
   %R1 G1 B1 R0 G0 B0). */
const MVTheme mv_theme_default = { {
    0x00, 0x07, 0x38, 0x3f,   /* 0-3  chrome: black, dark grey, light grey, white */
    0x24, 0x12, 0x09, 0x36,   /* 4-7  content: red, green, blue, yellow */
    0x1b, 0x2d, 0x20, 0x10,   /* 8-11 content: cyan, magenta, dark red, dark green */
    0x08, 0x04, 0x07, 0x38    /* 12-15 content: dark blue, dim red, dark grey, light grey */
} };
