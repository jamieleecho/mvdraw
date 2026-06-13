#include <cgfx.h>
#include <mvkit/mv_defs.h>   /* MV_OUTPATH, Flush */

#include "draw_view.h"


/* Selection-handle half-size and the pick tolerance (canvas pixels). */
#define HANDLE_HALF  2
#define PICK_TOL     4
/* Hit-test margin around a shape's bounding box when selecting. */
#define SHAPE_TOL    3


/* ---- small helpers ------------------------------------------------------ */

static int imin(int a, int b) { return a < b ? a : b; }
static int imax(int a, int b) { return a > b ? a : b; }
static int iabs(int a) { return a < 0 ? -a : a; }

static int clampi(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

/* floor(sqrt(n)) by the bit-by-bit method -- no division, no float. n is a
   32-bit value; the result fits an int (a canvas diagonal is < 600). */
static int isqrt32(unsigned long n) {
    unsigned long bit = 0x40000000UL;   /* largest power of 4 <= 2^31 */
    unsigned long root = 0;
    while (bit > n) {
        bit >>= 2;
    }
    while (bit) {
        if (n >= root + bit) {
            n -= root + bit;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return (int)root;
}

/* The canvas is clipped to its own working area while drawing so shapes (esp.
   circles, which extend from a center) never spill onto the toolbars. cwarea
   coordinates are 8x8 character cells; the canvas origin/size are multiples of
   CELL by construction (mvdraw.c). Setting the working area ALSO shifts the
   origin to the canvas corner, so all canvas drawing below is done in
   canvas-relative coords ((0,0) == canvas top-left); the window-relative mouse
   is converted on the way in. Scaling must be off (mvdraw_init) so 1 unit ==
   1 pixel. */
#define CELL  8

/* The window's full working area in character cells (its content excludes a
   one-cell border), restored after canvas drawing so the framework's menus,
   dialogs and toolbars draw unclipped. */
#define WINDOW_CCOL  1
#define WINDOW_CROW  1
#define WINDOW_CCW   78
#define WINDOW_CCH   23

static void clip_to_canvas(const DrawView *v) {
    /* view.x/y are relative to the standard working area's origin (cell
       WINDOW_CCOL,WINDOW_CROW), so add that origin to get the canvas's absolute
       cell position -- otherwise the canvas clips/draws one cell off. */
    _cgfx_cwarea(MV_OUTPATH,
                 v->view.x / CELL + WINDOW_CCOL, v->view.y / CELL + WINDOW_CROW,
                 v->view.width / CELL, v->view.height / CELL);
}

static void clip_reset(void) {
    _cgfx_cwarea(MV_OUTPATH, WINDOW_CCOL, WINDOW_CROW, WINDOW_CCW, WINDOW_CCH);
}

/* Clamp a canvas-relative point to the content rectangle. */
static void clamp_to_canvas(const DrawView *v, int *x, int *y) {
    *x = clampi(*x, 0, v->view.width - 1);
    *y = clampi(*y, 0, v->view.height - 1);
}


/* ---- shape rendering ---------------------------------------------------- */

/* Draw the geometry of `type` spanning screen corners (x0,y0)-(x1,y1). For box
   types corner order does not matter; for a line the two points are endpoints.
   The caller sets color / pattern / logic first. */
static void stroke_geometry(int type, int x0, int y0, int x1, int y1) {
    int cx, cy, xr, yr, r;

    switch (type) {
        case SHAPE_RECT:
            _cgfx_setdptr(MV_OUTPATH, x0, y0);
            _cgfx_box(MV_OUTPATH, x1, y1);
            break;
        case SHAPE_FRECT:
            _cgfx_setdptr(MV_OUTPATH, x0, y0);
            _cgfx_bar(MV_OUTPATH, x1, y1);
            break;
        case SHAPE_CIRCLE:
            cx = (x1 - x0);
            cy = (y1 - y0) * 2;
            r = isqrt32((unsigned long)cx * cx + (unsigned long)cy * cy);
            _cgfx_setdptr(MV_OUTPATH, x0, y0);
            _cgfx_circle(MV_OUTPATH, r);
            break;
        case SHAPE_ELLIPSE:
            cx = (x0 + x1) / 2;
            cy = (y0 + y1) / 2;
            xr = iabs(x1 - x0) / 2;
            yr = iabs(y1 - y0) / 2;
            _cgfx_setdptr(MV_OUTPATH, cx, cy);
            _cgfx_ellipse(MV_OUTPATH, xr, yr);
            break;
        case SHAPE_LINE:
            _cgfx_setdptr(MV_OUTPATH, x0, y0);
            _cgfx_line(MV_OUTPATH, x1, y1);
            break;
    }
}

/* Render a stored shape (canvas-relative), honoring its color/pattern/logic. */
static void render_shape(const DrawView *v, const Shape *s) {
    _cgfx_lset(MV_OUTPATH, s->logic);
    _cgfx_fcolor(MV_OUTPATH, s->color);     /* ink for the pattern's set bits */
    _cgfx_bcolor(MV_OUTPATH, v->paper);     /* paper shows through the gaps */
    _cgfx_pset(MV_OUTPATH, s->pattern ? GRP_PAT2 : 0, s->pattern);
    stroke_geometry((int)s->type, s->x0, s->y0, s->x1, s->y1);
    _cgfx_pset(MV_OUTPATH, 0, PAT_SLD);   /* leave solid for paper/handles */
    _cgfx_lset(MV_OUTPATH, LOG_NONE);
}

/* Draw (or, called twice, erase) an XOR rubber-band outline. For a filled rect
   the preview is just the outline -- cheap, since the CoCo is slow. */
static void xor_preview(const DrawView *v, int type, int x0, int y0, int x1, int y1) {
    int t = (type == SHAPE_FRECT) ? SHAPE_RECT : type;
    /* Self-clip: the canvas working area is set only around the draw, never
       while the mouse is read, so mouse coordinates stay in one space. */
    clip_to_canvas(v);
    _cgfx_lset(MV_OUTPATH, LOG_XOR);
    _cgfx_fcolor(MV_OUTPATH, v->preview_color);   /* color 1: XOR toggles pixels */
    stroke_geometry(t, x0, y0, x1, y1);
    _cgfx_lset(MV_OUTPATH, LOG_NONE);
    clip_reset();
}


/* ---- selection handles -------------------------------------------------- */

/* Corner points (screen coords) of a shape's selection handles. For a box type
   the four bbox corners; for a line the two endpoints (slots 0 and 3). Returns
   the number of handles and fills hx[4]/hy[4]. */
static int handle_points(const DrawView *v, const Shape *s, int *hx, int *hy) {
    int x0 = s->x0, y0 = s->y0;   /* canvas-relative */
    int x1 = s->x1, y1 = s->y1;
    (void)v;

    if (s->type == SHAPE_LINE) {
        hx[0] = x0; hy[0] = y0;
        hx[3] = x1; hy[3] = y1;
        hx[1] = hy[1] = hx[2] = hy[2] = -10000;   /* unused slots */
        return 4;
    } else if (s->type == SHAPE_CIRCLE) {
        int r = isqrt32((unsigned long)(x1 - x0) * (x1 - x0) +
                     (unsigned long)(y1 - y0) * (y1 - y0));
        int r2 = r / 2;
        hx[0] = x0 - r; hy[0] = y0 - r2;   /* N */
        hx[1] = x0 + r; hy[1] = y0 - r2;   /* E */
        hx[2] = x0 + r; hy[2] = y0 + r2;   /* S */
        hx[3] = x0 - r; hy[3] = y0 + r2;   /* W */
    } else {
        hx[0] = x0; hy[0] = y0;   /* TL */
        hx[1] = x1; hy[1] = y0;   /* TR */
        hx[2] = x0; hy[2] = y1;   /* BL */
        hx[3] = x1; hy[3] = y1;   /* BR */
    }
    return 4;
}

static void draw_handle(const DrawView *v, int cx, int cy) {
    _cgfx_lset(MV_OUTPATH, LOG_NONE);
    _cgfx_fcolor(MV_OUTPATH, v->handle_color);
    _cgfx_setdptr(MV_OUTPATH, cx - HANDLE_HALF, cy - HANDLE_HALF);
    _cgfx_rbar(MV_OUTPATH, HANDLE_HALF * 2, HANDLE_HALF * 2);
}

static void draw_handles(const DrawView *v) {
    int hx[4], hy[4], i;
    const Shape *s = &v->drawing->shapes[v->selected];
    handle_points(v, s, hx, hy);
    for (i = 0; i < 4; ++i) {
        if (s->type == SHAPE_LINE && (i == 1 || i == 2)) {
            continue;
        }
        draw_handle(v, hx[i], hy[i]);
    }
}


/* ---- hit testing -------------------------------------------------------- */

/* Returns the handle index (0..3) near screen point (sx,sy) for the selected
   shape, or -1. */
static int hit_handle(const DrawView *v, int sx, int sy) {
    int hx[4], hy[4], i;
    const Shape *s;

    if (v->selected < 0 || v->selected >= v->drawing->num_shapes) {
        return -1;
    }
    s = &v->drawing->shapes[v->selected];
    handle_points(v, s, hx, hy);
    for (i = 0; i < 4; ++i) {
        if (s->type == SHAPE_LINE && (i == 1 || i == 2)) {
            continue;
        }
        if (iabs(sx - hx[i]) <= PICK_TOL && iabs(sy - hy[i]) <= PICK_TOL) {
            return i;
        }
    }
    return -1;
}

/* True if canvas point (cx,cy) is within `tol` pixels of the line *segment*
   (x0,y0)-(x1,y1) -- a thin band hugging the line, not its bounding box.

   All intermediates are 32-bit: a cross product reaches ~|dx|*|dy| (~87k on
   this canvas), which overflows 16-bit. We avoid the squared-distance compare
   (cross*cross would overflow even 32-bit) by taking an integer sqrt of the
   length and comparing |cross| <= tol*len directly. */
static bool hit_line(const Shape *s, int cx, int cy, int tol) {
    long dx = (long)s->x1 - s->x0;
    long dy = (long)s->y1 - s->y0;
    long len2 = dx * dx + dy * dy;
    long t, cross;

    if (len2 == 0) {                    /* degenerate line == point */
        return iabs(cx - s->x0) <= tol && iabs(cy - s->y0) <= tol;
    }

    /* Reject points off the ends: the projection of (P-A) onto (B-A) must land
       within [0, len2], so the band has square caps at the endpoints. */
    t = (long)(cx - s->x0) * dx + (long)(cy - s->y0) * dy;
    if (t < 0 || t > len2) {
        return false;
    }

    /* Perpendicular distance from the line is |cross|/len; within tol when
       |cross| <= tol*len. */
    cross = (long)(cx - s->x0) * dy - (long)(cy - s->y0) * dx;
    if (cross < 0) {
        cross = -cross;
    }
    return cross <= (long)tol * isqrt32((unsigned long)len2);
}

/* Aspect-corrected radius of a circle shape, matching stroke_geometry(): the
   circle is centered at (x0,y0) with horizontal radius r and (because type-5
   pixels are ~2x taller than wide) vertical radius r/2. */
static int circle_radius(const Shape *s) {
    long dx = (long)s->x1 - s->x0;
    long dy = ((long)s->y1 - s->y0) * 2;
    return isqrt32((unsigned long)(dx * dx + dy * dy));
}

/* Returns the top-most shape hit by canvas point (cx,cy), or -1. Lines use a
   thin band along the segment; other shapes use their bounding box plus a
   margin. */
static int hit_shape(const DrawView *v, int cx, int cy) {
    int i;
    for (i = v->drawing->num_shapes - 1; i >= 0; --i) {
        const Shape *s = &v->drawing->shapes[i];
        int lo_x, hi_x, lo_y, hi_y;
        if (s->type == SHAPE_LINE) {
            if (hit_line(s, cx, cy, SHAPE_TOL)) {
                return i;
            }
            continue;
        }
        if (s->type == SHAPE_CIRCLE) {
            /* Centered at (x0,y0): extends +/-r horizontally, +/-r/2 vertically. */
            int r = circle_radius(s);
            lo_x = s->x0 - r - SHAPE_TOL;
            hi_x = s->x0 + r + SHAPE_TOL;
            lo_y = s->y0 - r / 2 - SHAPE_TOL;
            hi_y = s->y0 + r / 2 + SHAPE_TOL;
            if (cx >= lo_x && cx <= hi_x && cy >= lo_y && cy <= hi_y) {
                return i;
            }
            continue;
        }
        lo_x = imin(s->x0, s->x1) - SHAPE_TOL;
        hi_x = imax(s->x0, s->x1) + SHAPE_TOL;
        lo_y = imin(s->y0, s->y1) - SHAPE_TOL;
        hi_y = imax(s->y0, s->y1) + SHAPE_TOL;
        if (cx >= lo_x && cx <= hi_x && cy >= lo_y && cy <= hi_y) {
            return i;
        }
    }
    return -1;
}


/* ---- full repaint ------------------------------------------------------- */

/* Paint the whole canvas (paper + shapes + handles) in canvas-relative coords.
   The caller must have the canvas clip/working area active (clip_to_canvas). */
static void render_all(DrawView *v) {
    int i;

    _cgfx_lset(MV_OUTPATH, LOG_NONE);
    _cgfx_fcolor(MV_OUTPATH, v->paper);
    _cgfx_setdptr(MV_OUTPATH, 0, 0);
    _cgfx_rbar(MV_OUTPATH, v->view.width - 1, v->view.height - 1);

    for (i = 0; i < v->drawing->num_shapes; ++i) {
        render_shape(v, &v->drawing->shapes[i]);
    }
    if (v->selected >= 0 && v->selected < v->drawing->num_shapes) {
        draw_handles(v);
    }
    _cgfx_lset(MV_OUTPATH, LOG_NONE);
}

/* The MVView draw callback: full repaint, self-bracketed by the canvas clip. */
static void draw_view_draw(MVView *mv) {
    DrawView *v = (DrawView *)mv;
    clip_to_canvas(v);
    render_all(v);
    clip_reset();
    Flush();
}


/* ---- drag tracking ------------------------------------------------------ */

/* Follow the mouse until button A releases, repainting an XOR preview whose
   anchor stays at (ax,ay) and whose moving end tracks the cursor (clamped to
   the canvas). All coordinates are canvas-relative; the window-relative mouse
   is converted here. On return *ex/*ey hold the final moving point. `type`
   selects the preview shape. The mouse is read with the standard working area
   active; xor_preview() sets the canvas clip only around each draw. */
static void track_drag(DrawView *v, MSRET *mp, int type,
                       int ax, int ay, int *ex, int *ey) {
    int mx = *ex, my = *ey;

    xor_preview(v, type, ax, ay, mx, my);
    Flush();
    do {
        _cgfx_gs_mouse(MV_OUTPATH, mp);
        {
            int nx = mp->pt_wrx - v->view.x, ny = mp->pt_wry - v->view.y;
            clamp_to_canvas(v, &nx, &ny);
            if (nx != mx || ny != my) {
                xor_preview(v, type, ax, ay, mx, my);   /* erase */
                mx = nx; my = ny;
                xor_preview(v, type, ax, ay, mx, my);   /* redraw */
                Flush();
            }
        }
    } while (mp->pt_cbsa);
    xor_preview(v, type, ax, ay, mx, my);               /* erase final */
    Flush();

    *ex = mx;
    *ey = my;
}

static void draw_handles(const DrawView *v);


/* Create a new shape with the current tool. (ax,ay) is the button-down point,
   canvas-relative. Drawing sets the canvas clip itself; the mouse is read with
   the standard working area active. */
static bool create_drag(DrawView *v, MSRET *mp, int ax, int ay) {
    int type = v->tool - TOOL_RECT + SHAPE_RECT;
    int ex = ax, ey = ay;
    Shape s;
    int idx;

    track_drag(v, mp, type, ax, ay, &ex, &ey);

    s.type = (unsigned char)type;
    s.pattern = v->pattern;
    s.color = v->color;
    s.logic = v->logic;
    if (type == SHAPE_LINE || type == SHAPE_CIRCLE) {
        /* Anchor-based: (x0,y0) is the start/center, (x1,y1) the drag point --
           matching stroke_geometry(), which centers the circle on (x0,y0). */
        s.x0 = ax; s.y0 = ay;
        s.x1 = ex; s.y1 = ey;
    } else {
        s.x0 = imin(ax, ex); s.y0 = imin(ay, ey);
        s.x1 = imax(ax, ex); s.y1 = imax(ay, ey);
    }
    if (s.x0 == s.x1 && s.y0 == s.y1) {
        draw_view_draw(&v->view);   /* degenerate: nothing to add */
        return true;
    }

    idx = drawing_add_shape(v->drawing, &s);
    if (idx >= 0) {
        v->selected = idx;
        if (v->on_add) {
            v->on_add(v, idx);
        }
        /* The existing shapes are untouched (the XOR preview erased itself) and
           the new shape is topmost, so just paint it -- no full repaint. */
        clip_to_canvas(v);
        render_shape(v, &v->drawing->shapes[idx]);
        draw_handles(v);
        clip_reset();
        Flush();
    }
    return true;
}


/* Resize the selected shape by dragging handle `h`. */
static bool resize_drag(DrawView *v, MSRET *mp, int idx, int h) {
    Shape *sp = &v->drawing->shapes[idx];
    int ax, ay;           /* fixed anchor, canvas-relative */
    int ex, ey;           /* moving point (starts at the grabbed handle) */
    Shape old;
    int hx[4], hy[4];

    handle_points(v, sp, hx, hy);
    ex = hx[h]; ey = hy[h];

    if (sp->type == SHAPE_LINE) {
        /* anchor is the opposite endpoint (handle 0 <-> 3) */
        int other = (h == 0) ? 3 : 0;
        ax = hx[other]; ay = hy[other];
    } else {
        /* anchor is the diagonally opposite corner: 0<->3, 1<->2 */
        int other = 3 - h;
        ax = hx[other]; ay = hy[other];
    }

    track_drag(v, mp, sp->type, ax, ay, &ex, &ey);

    old = *sp;
    if (sp->type == SHAPE_LINE) {
        if (h == 0) { sp->x0 = ex; sp->y0 = ey; }
        else        { sp->x1 = ex; sp->y1 = ey; }
    } else {
        sp->x0 = imin(ax, ex); sp->y0 = imin(ay, ey);
        sp->x1 = imax(ax, ex); sp->y1 = imax(ay, ey);
    }

    if (sp->x0 != old.x0 || sp->y0 != old.y0 ||
        sp->x1 != old.x1 || sp->y1 != old.y1) {
        void *rec = drawing_record_edit(v->drawing, idx, &old);
        if (v->on_edit) {
            v->on_edit(v, idx, rec);
        }
    }
    draw_view_draw(&v->view);
    return true;
}


/* Move the selected shape by dragging its body. (sx,sy) is the button-down,
   canvas-relative. The mouse is read with the standard area active. */
static bool move_drag(DrawView *v, MSRET *mp, int idx, int sx, int sy) {
    Shape *sp = &v->drawing->shapes[idx];
    int x0 = sp->x0, y0 = sp->y0;   /* canvas-relative */
    int x1 = sp->x1, y1 = sp->y1;
    int lo_x = imin(sp->x0, sp->x1), hi_x = imax(sp->x0, sp->x1);
    int lo_y = imin(sp->y0, sp->y1), hi_y = imax(sp->y0, sp->y1);
    int dx = 0, dy = 0;
    Shape old;

    xor_preview(v, sp->type, x0, y0, x1, y1);
    Flush();
    do {
        _cgfx_gs_mouse(MV_OUTPATH, mp);
        {
            /* convert mouse to canvas-relative, then clamp the translation so
               the whole shape stays on the canvas */
            int mcx = mp->pt_wrx - v->view.x, mcy = mp->pt_wry - v->view.y;
            int ndx = clampi(mcx - sx, -lo_x, (v->view.width - 1) - hi_x);
            int ndy = clampi(mcy - sy, -lo_y, (v->view.height - 1) - hi_y);
            if (ndx != dx || ndy != dy) {
                xor_preview(v, sp->type, x0 + dx, y0 + dy, x1 + dx, y1 + dy);
                dx = ndx; dy = ndy;
                xor_preview(v, sp->type, x0 + dx, y0 + dy, x1 + dx, y1 + dy);
                Flush();
            }
        }
    } while (mp->pt_cbsa);
    xor_preview(v, sp->type, x0 + dx, y0 + dy, x1 + dx, y1 + dy);
    Flush();

    if (dx != 0 || dy != 0) {
        old = *sp;
        sp->x0 += dx; sp->y0 += dy;
        sp->x1 += dx; sp->y1 += dy;
        {
            void *rec = drawing_record_edit(v->drawing, idx, &old);
            if (v->on_edit) {
                v->on_edit(v, idx, rec);
            }
        }
    }
    draw_view_draw(&v->view);
    return true;
}


/* Select-tool press: handle resize, then select+move, then deselect. (cx,cy)
   is the button-down, canvas-relative. */
static bool select_press(DrawView *v, MSRET *mp, int cx, int cy) {
    int h, idx;

    h = hit_handle(v, cx, cy);
    if (h >= 0) {
        return resize_drag(v, mp, v->selected, h);
    }

    idx = hit_shape(v, cx, cy);
    if (idx >= 0) {
        if (idx != v->selected) {
            v->selected = idx;
            draw_view_draw(&v->view);   /* show the new selection's handles */
        }
        return move_drag(v, mp, idx, cx, cy);
    }

    if (v->selected >= 0) {
        v->selected = -1;
        draw_view_draw(&v->view);
    }
    return true;
}


static bool draw_view_handle_click(MVView *mv, MVUiEvent *event) {
    DrawView *v = (DrawView *)mv;
    MSRET mp;
    int wx, wy, cx, cy;
    bool result;

    if (event->event_type != MVUiEventType_MouseClick) {
        return false;
    }
    mp = event->info.mouse;
    wx = mp.pt_wrx;
    wy = mp.pt_wry;
    if (!mv_view_contains_point(mv, wx, wy)) {
        return false;
    }
    /* Window-relative -> canvas-relative. The canvas working area shifts the
       drawing origin to its corner; we set it ONLY around drawing (in the draw
       primitives below), never while reading the mouse, so every mouse read --
       this event and the in-drag reads -- shares one coordinate space. */
    cx = wx - v->view.x;
    cy = wy - v->view.y;

    if (v->tool == TOOL_SELECT) {
        result = select_press(v, &mp, cx, cy);
    } else {
        result = create_drag(v, &mp, cx, cy);
    }
    return result;
}


/* ---- public API --------------------------------------------------------- */

void draw_view_init(DrawView *v, int x, int y, int width, int height,
                    Drawing *drawing) {
    v->view.x = x;
    v->view.y = y;
    v->view.width = width;
    v->view.height = height;
    v->view.is_visible = true;
    v->view.draw = draw_view_draw;
    v->view.handle_click = draw_view_handle_click;

    v->drawing = drawing;
    v->tool = TOOL_SELECT;
    v->pattern = PAT_SLD;
    v->color = 0;            /* black ink */
    v->logic = LOG_NONE;
    v->paper = 1;            /* white paper */
    v->preview_color = 1;    /* XOR with a set bit so it toggles pixels */
    v->handle_color = 0;     /* black handles on white paper */
    v->selected = -1;
    v->on_add = 0;
    v->on_edit = 0;
}

void draw_view_refresh(DrawView *v) {
    draw_view_draw(&v->view);
}

void draw_view_set_tool(DrawView *v, int tool) {
    bool was_select = (v->tool == TOOL_SELECT);
    v->tool = tool;
    /* Repaint so handles appear/disappear as the Select tool toggles. */
    if (was_select != (tool == TOOL_SELECT)) {
        draw_view_draw(&v->view);
    }
}

void draw_view_set_pattern(DrawView *v, unsigned char pattern) {
    v->pattern = pattern;
}

void draw_view_set_color(DrawView *v, unsigned char color) {
    v->color = color;
}

void draw_view_set_logic(DrawView *v, unsigned char logic) {
    v->logic = logic;
}
