#ifndef _DRAWING_H_
#define _DRAWING_H_

#include <os.h>   /* error_code */

/*
 * mvdraw's document model: an ordered list of vector shapes. Geometry is stored
 * canvas-relative (0,0 == canvas top-left) so a drawing is position-independent
 * -- the same idea as xmastree storing ornaments tree-relative.
 *
 * The model is framework-agnostic (it knows nothing about MVKit). The app
 * (mvdraw.c) wraps it in an MVDocument and builds MVUndoItems from the plain
 * undo functions below.
 */

#define DRAW_MAX_SHAPES 64

typedef enum {
    SHAPE_RECT = 0,   /* rectangle outline      (_cgfx_box)     */
    SHAPE_FRECT,      /* filled rectangle       (_cgfx_bar)     */
    SHAPE_CIRCLE,     /* circle outline         (_cgfx_circle)  */
    SHAPE_ELLIPSE,    /* ellipse outline        (_cgfx_ellipse) */
    SHAPE_LINE,       /* line segment           (_cgfx_line)    */
    SHAPE_TYPE_COUNT
} ShapeType;

/* One shape. (x0,y0)-(x1,y1) is the bounding box; for SHAPE_LINE it is the
   two endpoints. Stored canvas-relative. */
typedef struct {
    unsigned char type;      /* ShapeType */
    unsigned char pattern;   /* cgfx PAT_*  (fill/line texture) */
    unsigned char color;     /* foreground palette register */
    unsigned char logic;     /* cgfx LOG_*  (NONE/AND/OR/XOR) */
    int x0, y0, x1, y1;      /* bounding box / endpoints, canvas-relative */
} Shape;

typedef struct {
    int num_shapes;
    Shape shapes[DRAW_MAX_SHAPES];
} Drawing;


/* ---- lifecycle (used as MVDocument new/open/save callbacks) ------------- */

/* Reset to an empty drawing. Returns 0 (matches the new_model contract). */
extern error_code drawing_new(Drawing *drawing, const char *filename);

/* Load from filename; validates and falls back to empty on bad data. */
extern error_code drawing_open(Drawing *drawing, const char *filename);

/* Write the used prefix to filename. */
extern error_code drawing_save(const Drawing *drawing, const char *filename);


/* ---- editing ------------------------------------------------------------ */

/* Append shape; returns its index, or -1 if the drawing is full. */
extern int drawing_add_shape(Drawing *drawing, const Shape *shape);

extern int drawing_count(const Drawing *drawing);


/* ---- undo (plain void(*)(void*) functions; the app builds MVUndoItems) --- */

/* Undo for an "add": drops the last shape. object == the Drawing *. */
extern void drawing_undo_remove_last(void *drawing);

/* Record `old` (shape `index`'s geometry from before a resize/move) into an
   internal ring and return an opaque object pointer to pass as an
   MVUndoItem.object. Call at commit time with the pre-edit shape. */
extern void *drawing_record_edit(Drawing *drawing, int index, const Shape *old);

/* Undo for a "resize/move": restores the geometry captured by the matching
   drawing_record_edit(). object == that record pointer. */
extern void drawing_undo_restore(void *record);

#endif /* _DRAWING_H_ */
