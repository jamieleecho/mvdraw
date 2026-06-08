/*
 * Using MVKit, stage 3: window types, colors, and images.
 *
 * Installs a 16-color palette (an MVTheme), then loads a PNG that was converted
 * to an OS-9 image at build time and draws it.
 *
 * Two palettes have to agree: the MVTheme the app installs at run time
 * (mv_app_set_theme) and the assets/app-palette.txt the build uses to convert
 * PNGs into .i09 images. Both list the same 16 colors, so a pixel that was
 * "red" in the PNG ends up index 4, which the installed palette draws as red.
 */
#include <cgfx.h>

#include <mvkit/mvkit.h>

#define LOGO_BUFFER 2

/* cgfx color numbers (colno). Registers 0-3 are the window-chrome ramp
   (darkest -> lightest); 4-15 are content colors. These match the R,G,B values
   in assets/app-palette.txt entry-for-entry. */
static const MVTheme theme = { {
    0x00, 0x07, 0x38, 0x3f,   /* 0-3  black, dark grey, light grey, white */
    0x24, 0x12, 0x09, 0x36,   /* 4-7  red, green, blue, yellow */
    0x1b, 0x2d, 0x20, 0x10,   /* 8-11 cyan, magenta, dark red, dark green */
    0x08, 0x04, 0x07, 0x38    /* 12-15 dark blue, dim red, dark grey, light grey */
} };

mv_menu_none(palette_window, "palette");

static void palette_pre_init(int argc, char **argv) {
    mv_app_set_theme(&theme);          /* load all 16 palette registers */
    mv_image_init("palette");          /* resources live in /dd/SYS/palette/ */
    mv_image_load_resource("logo.i09", LOGO_BUFFER);
}

static void palette_init(void) {
    _cgfx_bcolor(MV_OUTPATH, 0);       /* background = chrome darkest (black) */
    _cgfx_clear(MV_OUTPATH);
    mv_image_draw(LOGO_BUFFER, 8, 8);  /* draw the converted image */
    Flush();
}

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &palette_window,
        palette_pre_init, palette_init, mv_app_menu_actions_nop,
        mv_app_refresh_menus_action_nop, mv_app_event_nop);
}
