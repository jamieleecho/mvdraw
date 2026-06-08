#include <fcntl.h>
#include <unistd.h>

#include "drawing.h"


/* Drawing coordinates are canvas-relative and modest; these bounds are a sanity
   check on loaded files (the canvas itself is well under 640x192). */
#define DRAW_MIN_COORD (-32)
#define DRAW_MAX_COORD  640


void drawing_init(Drawing *drawing) {
    drawing->num_shapes = 0;
}


error_code drawing_new(Drawing *drawing, const char *filename) {
    (void)filename;
    drawing->num_shapes = 0;
    return 0;
}


static error_code drawing_validate(const Drawing *drawing) {
    if ((drawing->num_shapes < 0) || (drawing->num_shapes > DRAW_MAX_SHAPES)) {
        return E$BMHP;
    }
    for (int ii = 0; ii < drawing->num_shapes; ++ii) {
        const Shape *s = drawing->shapes + ii;
        if (s->type >= SHAPE_TYPE_COUNT) {
            return E$BMHP;
        }
        if ((s->x0 < DRAW_MIN_COORD) || (s->x0 > DRAW_MAX_COORD) ||
            (s->y0 < DRAW_MIN_COORD) || (s->y0 > DRAW_MAX_COORD) ||
            (s->x1 < DRAW_MIN_COORD) || (s->x1 > DRAW_MAX_COORD) ||
            (s->y1 < DRAW_MIN_COORD) || (s->y1 > DRAW_MAX_COORD)) {
            return E$BMHP;
        }
    }
    return 0;
}


error_code drawing_open(Drawing *drawing, const char *filename) {
    int fd = open(filename, FAP_READ);
    if (fd < 0) {
        return errno;
    }
    int bytes_read = read(fd, (char *)drawing, sizeof(Drawing));
    error_code retval;
    if (bytes_read >= 0) {
        retval = drawing_validate(drawing);
    } else {
        retval = errno;
    }
    close(fd);

    if (retval) {
        drawing_init(drawing);
    }
    return retval;
}


error_code drawing_save(const Drawing *drawing, const char *filename) {
    int fd = creat(filename, FAP_WRITE);
    if (fd < 0) {
        return errno;
    }
    /* Write only the header plus the shapes in use. */
    int bytes_to_write = (char *)(&drawing->shapes[drawing->num_shapes]) - (char *)drawing;
    int bytes_written = write(fd, (const char *)drawing, bytes_to_write);
    error_code retval;
    if (bytes_written < 0) {
        retval = errno;
    } else if (bytes_written != bytes_to_write) {
        retval = E$Write;
    } else {
        retval = 0;
    }
    close(fd);
    return retval;
}


int drawing_add_shape(Drawing *drawing, const Shape *shape) {
    if (drawing->num_shapes >= DRAW_MAX_SHAPES) {
        return -1;
    }
    drawing->shapes[drawing->num_shapes] = *shape;
    return drawing->num_shapes++;
}


int drawing_count(const Drawing *drawing) {
    return drawing->num_shapes;
}


/* ---- undo --------------------------------------------------------------- */

void drawing_undo_remove_last(void *drawing) {
    Drawing *d = (Drawing *)drawing;
    if (d->num_shapes > 0) {
        --d->num_shapes;
    }
}


/* A small ring of geometry snapshots backs the resize/move undo items. The
   undo stack is itself bounded (MVKit caps it at 16), so a ring of the same
   size never aliases a still-live undo item. */
#define DRAW_EDIT_RING 16

typedef struct {
    Drawing *drawing;
    int index;
    Shape old;
} ShapeEdit;

static ShapeEdit edit_ring[DRAW_EDIT_RING];
static int edit_cursor;


void *drawing_record_edit(Drawing *drawing, int index, const Shape *old) {
    ShapeEdit *e = &edit_ring[edit_cursor];
    edit_cursor = (edit_cursor + 1) % DRAW_EDIT_RING;
    e->drawing = drawing;
    e->index = index;
    e->old = *old;
    return e;
}


void drawing_undo_restore(void *record) {
    ShapeEdit *e = (ShapeEdit *)record;
    if ((e->index >= 0) && (e->index < e->drawing->num_shapes)) {
        e->drawing->shapes[e->index] = e->old;
    }
}


void *drawing_delete_shape(Drawing *drawing, int index) {
    if ((index < 0) || (index >= drawing->num_shapes)) {
        return (void *)0;
    }
    /* Snapshot for undo before mutating. */
    void *record = drawing_record_edit(drawing, index, &drawing->shapes[index]);
    for (int ii = index; ii < drawing->num_shapes - 1; ++ii) {
        drawing->shapes[ii] = drawing->shapes[ii + 1];
    }
    --drawing->num_shapes;
    return record;
}


void drawing_undo_reinsert(void *record) {
    ShapeEdit *e = (ShapeEdit *)record;
    Drawing *d = e->drawing;
    if ((d->num_shapes >= DRAW_MAX_SHAPES) ||
        (e->index < 0) || (e->index > d->num_shapes)) {
        return;
    }
    for (int ii = d->num_shapes; ii > e->index; --ii) {
        d->shapes[ii] = d->shapes[ii - 1];
    }
    d->shapes[e->index] = e->old;
    ++d->num_shapes;
}
