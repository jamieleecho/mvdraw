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

/* Clamp a screen point to the view's content rectangle. */
static void clamp_to_view(const DrawView *v, int *x, int *y) {
    *x = clampi(*x, v->view.x, v->view.x + v->view.width - 1);
    *y = clampi(*y, v->view.y, v->view.y + v->view.height - 1);
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
            cx = (x0 + x1) / 2;
            cy = (y0 + y1) / 2;
            r = imin(iabs(x1 - x0), iabs(y1 - y0)) / 2;
            _cgfx_setdptr(MV_OUTPATH, cx, cy);
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
        default:
            break;
    }
}

/* Render a stored shape at the view's origin, honoring its color/pattern/logic. */
static void render_shape(const DrawView *v, const Shape *s) {
    int x0 = s->x0 + v->view.x, y0 = s->y0 + v->view.y;
    int x1 = s->x1 + v->view.x, y1 = s->y1 + v->view.y;

    _cgfx_lset(MV_OUTPATH, s->logic);
    _cgfx_bcolor(MV_OUTPATH, s->color);
    _cgfx_fcolor(MV_OUTPATH, s->color);
    stroke_geometry((int)s->type, x0, y0, x1, y1);
    _cgfx_lset(MV_OUTPATH, LOG_NONE);
}

/* Draw (or, called twice, erase) an XOR rubber-band outline. For a filled rect
   the preview is just the outline -- cheap, since the CoCo is slow. */
static void xor_preview(const DrawView *v, int type, int x0, int y0, int x1, int y1) {
    int t = (type == SHAPE_FRECT) ? SHAPE_RECT : type;
    _cgfx_lset(MV_OUTPATH, LOG_XOR);
    _cgfx_fcolor(MV_OUTPATH, v->preview_color);   /* color 1: XOR toggles pixels */
    stroke_geometry(t, x0, y0, x1, y1);
    _cgfx_lset(MV_OUTPATH, LOG_NONE);
}


/* ---- selection handles -------------------------------------------------- */

/* Corner points (screen coords) of a shape's selection handles. For a box type
   the four bbox corners; for a line the two endpoints (slots 0 and 3). Returns
   the number of handles and fills hx[4]/hy[4]. */
static int handle_points(const DrawView *v, const Shape *s, int *hx, int *hy) {
    int x0 = s->x0 + v->view.x, y0 = s->y0 + v->view.y;
    int x1 = s->x1 + v->view.x, y1 = s->y1 + v->view.y;

    if (s->type == SHAPE_LINE) {
        hx[0] = x0; hy[0] = y0;
        hx[3] = x1; hy[3] = y1;
        hx[1] = hy[1] = hx[2] = hy[2] = -10000;   /* unused slots */
        return 4;
    }
    hx[0] = x0; hy[0] = y0;   /* TL */
    hx[1] = x1; hy[1] = y0;   /* TR */
    hx[2] = x0; hy[2] = y1;   /* BL */
    hx[3] = x1; hy[3] = y1;   /* BR */
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

/* Returns the top-most shape whose bounding box (plus margin) contains canvas
   point (cx,cy), or -1. */
static int hit_shape(const DrawView *v, int cx, int cy) {
    int i;
    for (i = v->drawing->num_shapes - 1; i >= 0; --i) {
        const Shape *s = &v->drawing->shapes[i];
        int lo_x = imin(s->x0, s->x1) - SHAPE_TOL;
        int hi_x = imax(s->x0, s->x1) + SHAPE_TOL;
        int lo_y = imin(s->y0, s->y1) - SHAPE_TOL;
        int hi_y = imax(s->y0, s->y1) + SHAPE_TOL;
        if (cx >= lo_x && cx <= hi_x && cy >= lo_y && cy <= hi_y) {
            return i;
        }
    }
    return -1;
}


/* ---- full repaint ------------------------------------------------------- */

static void draw_view_draw(MVView *mv) {
    DrawView *v = (DrawView *)mv;
    int i;

    _cgfx_lset(MV_OUTPATH, LOG_NONE);
    _cgfx_fcolor(MV_OUTPATH, v->paper);
    _cgfx_setdptr(MV_OUTPATH, v->view.x, v->view.y);
    _cgfx_rbar(MV_OUTPATH, v->view.width-1, v->view.height-1);

    for (i = 0; i < v->drawing->num_shapes; ++i) {
        render_shape(v, &v->drawing->shapes[i]);
    }
    if (v->tool == TOOL_SELECT &&
        v->selected >= 0 && v->selected < v->drawing->num_shapes) {
        draw_handles(v);
    }
    _cgfx_lset(MV_OUTPATH, LOG_NONE);
    Flush();
}


/* ---- drag tracking ------------------------------------------------------ */

/* Follow the mouse until button A releases, repainting an XOR preview whose
   anchor stays at (ax,ay) and whose moving end tracks the cursor (clamped to
   the canvas). On return *ex/*ey hold the final moving point (screen coords).
   `type` selects the preview shape. */
static void track_drag(DrawView *v, MSRET *mp, int type,
                       int ax, int ay, int *ex, int *ey) {
    int mx = *ex, my = *ey;

    xor_preview(v, type, ax, ay, mx, my);
    Flush();
    do {
        _cgfx_gs_mouse(MV_OUTPATH, mp);
        {
            int nx = mp->pt_wrx, ny = mp->pt_wry;
            clamp_to_view(v, &nx, &ny);
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


/* Create a new shape with the current tool. (ax,ay) is the button-down point. */
static bool create_drag(DrawView *v, MSRET *mp, int ax, int ay) {
    int type = v->tool - TOOL_RECT + SHAPE_RECT;
    int ex = ax, ey = ay;
    int vx = v->view.x, vy = v->view.y;
    Shape s;
    int idx;

    track_drag(v, mp, type, ax, ay, &ex, &ey);

    s.type = (unsigned char)type;
    s.pattern = v->pattern;
    s.color = v->color;
    s.logic = v->logic;
    if (type == SHAPE_LINE) {
        s.x0 = ax - vx; s.y0 = ay - vy;
        s.x1 = ex - vx; s.y1 = ey - vy;
    } else {
        s.x0 = imin(ax, ex) - vx; s.y0 = imin(ay, ey) - vy;
        s.x1 = imax(ax, ex) - vx; s.y1 = imax(ay, ey) - vy;
        if (s.x0 == s.x1 && s.y0 == s.y1) {
            draw_view_draw(&v->view);   /* degenerate: nothing to add */
            return true;
        }
    }

    idx = drawing_add_shape(v->drawing, &s);
    if (idx >= 0) {
        v->selected = idx;
        if (v->on_add) {
            v->on_add(v, idx);
        }
    }

    draw_view_draw(&v->view);
    return true;
}


/* Resize the selected shape by dragging handle `h`. */
static bool resize_drag(DrawView *v, MSRET *mp, int idx, int h) {
    Shape *sp = &v->drawing->shapes[idx];
    int vx = v->view.x, vy = v->view.y;
    int ax, ay;           /* fixed anchor, screen */
    int ex, ey;           /* moving point, screen (starts at the grabbed handle) */
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
        if (h == 0) { sp->x0 = ex - vx; sp->y0 = ey - vy; }
        else        { sp->x1 = ex - vx; sp->y1 = ey - vy; }
    } else {
        sp->x0 = imin(ax, ex) - vx; sp->y0 = imin(ay, ey) - vy;
        sp->x1 = imax(ax, ex) - vx; sp->y1 = imax(ay, ey) - vy;
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


/* Move the selected shape by dragging its body. (sx,sy) is the button-down. */
static bool move_drag(DrawView *v, MSRET *mp, int idx, int sx, int sy) {
    Shape *sp = &v->drawing->shapes[idx];
    int x0 = sp->x0 + v->view.x, y0 = sp->y0 + v->view.y;
    int x1 = sp->x1 + v->view.x, y1 = sp->y1 + v->view.y;
    int lo_x = imin(sp->x0, sp->x1), hi_x = imax(sp->x0, sp->x1);
    int lo_y = imin(sp->y0, sp->y1), hi_y = imax(sp->y0, sp->y1);
    int dx = 0, dy = 0;
    Shape old;

    xor_preview(v, sp->type, x0, y0, x1, y1);
    Flush();
    do {
        _cgfx_gs_mouse(MV_OUTPATH, mp);
        {
            /* clamp the translation so the whole shape stays on the canvas */
            int ndx = clampi(mp->pt_wrx - sx, -lo_x, (v->view.width - 1) - hi_x);
            int ndy = clampi(mp->pt_wry - sy, -lo_y, (v->view.height - 1) - hi_y);
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


/* Select-tool press: handle resize, then select+move, then deselect. */
static bool select_press(DrawView *v, MSRET *mp, int sx, int sy) {
    int cx = sx - v->view.x, cy = sy - v->view.y;
    int h, idx;

    h = hit_handle(v, sx, sy);
    if (h >= 0) {
        return resize_drag(v, mp, v->selected, h);
    }

    idx = hit_shape(v, cx, cy);
    if (idx >= 0) {
        if (idx != v->selected) {
            v->selected = idx;
            draw_view_draw(&v->view);   /* show the new selection's handles */
        }
        return move_drag(v, mp, idx, sx, sy);
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
    int sx, sy;

    if (event->event_type != MVUiEventType_MouseClick) {
        return false;
    }
    mp = event->info.mouse;
    sx = mp.pt_wrx;
    sy = mp.pt_wry;
    if (!mv_view_contains_point(mv, sx, sy)) {
        return false;
    }

    if (v->tool == TOOL_SELECT) {
        return select_press(v, &mp, sx, sy);
    }
    return create_drag(v, &mp, sx, sy);
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
    v->tool = TOOL_RECT;
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
