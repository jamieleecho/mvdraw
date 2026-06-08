/*
 * Using MVKit, stage 6: a document.
 *
 * mv_document wraps an app's data model with the standard file lifecycle --
 * New / Open / Save / Save As / Revert -- plus dirty tracking and the
 * save-before-discarding prompts, loosely after NSDocument.
 *
 * The model here is the simplest thing that can be saved: a single integer
 * counter. You supply three callbacks that act on the model for a path
 * (new/open/save); MVKit drives them from the File menu, tracks whether there
 * are unsaved changes, and prompts before throwing work away.
 *
 * The "Count > Increment" item changes the model. Every change is recorded with
 * mv_document_make_change, which marks the document dirty and remembers how to
 * undo it -- the next stage wires up an Undo menu that uses that.
 */
#include <fcntl.h>      /* open, creat, FAP_READ, FAP_WRITE */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <errno.h>

#include <mvkit/mvkit.h>

#define MN_COUNT 30     /* app-chosen menu id (cgfx reserves the low numbers) */

/* The app's data model -- opaque to MVKit, which only ever hands it back to the
   callbacks below as a void *. */
typedef struct {
    int count;
} Counter;

static Counter counter;
static MVDocument doc;

/* The three model callbacks. Each acts on the model for a path and returns 0 on
   success or an error code; any may be NULL to disable that operation. */

static int counter_new(void *model, const char *path) {
    (void)path;
    ((Counter *)model)->count = 0;
    return 0;
}

static int counter_open(void *model, const char *path) {
    Counter *c = (Counter *)model;
    int fd = open(path, FAP_READ);
    if (fd < 0) {
        return errno;
    }
    int n = read(fd, (char *)&c->count, sizeof(c->count));
    close(fd);
    if (n != (int)sizeof(c->count)) {
        return (n < 0) ? errno : E$Read;
    }
    return 0;
}

static int counter_save(void *model, const char *path) {
    Counter *c = (Counter *)model;
    int fd = creat(path, FAP_WRITE);
    if (fd < 0) {
        return errno;
    }
    int n = write(fd, (const char *)&c->count, sizeof(c->count));
    close(fd);
    if (n != (int)sizeof(c->count)) {
        return (n < 0) ? errno : E$Write;
    }
    return 0;
}

/* Undo for one Increment: put the count back. (Used by stage 7's Undo menu.) */
static void counter_undo_increment(void *object) {
    ((Counter *)object)->count -= 1;
}

/* Menus. The File menu uses the standard cgfx id MN_FILE so it reads as the
   real File menu; item numbers below are 1-based and count every row. */
typedef enum {
    FileIndex_Save   = 2,   /* 0-based array index of "Save" */
    FileIndex_Revert = 4,   /* 0-based array index of "Revert" */
} FileIndex;

static MIDSCR file_items[] = {
    MV_MENU_ITEM("New"),          /* item 1 */
    MV_MENU_ITEM("Open..."),      /* item 2 */
    MV_MENU_ITEM("Save"),         /* item 3 */
    MV_MENU_ITEM("Save As..."),   /* item 4 */
    MV_MENU_ITEM("Revert"),       /* item 5 */
};

static MIDSCR count_items[] = {
    MV_MENU_ITEM("Increment"),    /* item 1 */
};

static MNDSCR menus[] = {
    MV_MENU("File", MN_FILE, file_items),
    MV_MENU("Count", MN_COUNT, count_items),
};

mv_set_menus(doc_window, "document", menus);


static void show(void) {
    printf("count = %d   [%s]   %s\n",
           counter.count,
           mv_document_is_dirty(&doc) ? "modified" : "saved",
           doc.path);
    Flush();
}


static void new_action(MSRET *msinfo, int menuid, int itemno) {
    if (mv_document_new(&doc)) {
        show();
    }
}

static void open_action(MSRET *msinfo, int menuid, int itemno) {
    if (mv_document_open(&doc)) {
        show();
    }
}

static void save_action(MSRET *msinfo, int menuid, int itemno) {
    mv_document_save(&doc);
    show();
}

static void save_as_action(MSRET *msinfo, int menuid, int itemno) {
    mv_document_save_as(&doc);
    show();
}

static void revert_action(MSRET *msinfo, int menuid, int itemno) {
    mv_document_revert(&doc);
    show();
}

static void increment_action(MSRET *msinfo, int menuid, int itemno) {
    MVUndoItem undo = { counter_undo_increment, &counter };
    counter.count += 1;
    mv_document_make_change(&doc, &undo);   /* marks dirty + records the undo */
    show();
}

/* The window's close box. Force-save a dirty document, then quit; if the save
   is cancelled or fails, stay open. */
static void close_action(MSRET *msinfo, int menuid, int itemno) {
    if (mv_document_is_dirty(&doc) && mv_document_save(&doc) != 0) {
        return;
    }
    exit(0);
}

static const MVMenuItemAction menu_actions[] = {
    {MN_FILE, 1, new_action},
    {MN_FILE, 2, open_action},
    {MN_FILE, 3, save_action},
    {MN_FILE, 4, save_as_action},
    {MN_FILE, 5, revert_action},
    {MN_COUNT, 1, increment_action},
    {MN_CLOS, 1, close_action},
    MV_MENU_ACTION_END
};


static void document_pre_init(int argc, char **argv) {
    mv_document_init(&doc, NULL, "counter", ".cnt", &counter,
                     counter_new, counter_open, counter_save);
    if (argc == 2) {                       /* a file named on the command line */
        counter_open(&counter, argv[1]);
        mv_document_opened(&doc);
    }
}

static void document_init(void) {
    printf("File > New/Open/Save/Revert; Count > Increment.\n");
    show();
}

/* Enable Save only when there are unsaved changes, and Revert only when the
   document is backed by a file we can reload. mv_app_run calls this whenever the
   menu bar is about to be shown. */
static void refresh_menus(void) {
    mv_menu_item_set_enabled(file_items, FileIndex_Save, mv_document_is_dirty(&doc));
    mv_menu_item_set_enabled(file_items, FileIndex_Revert, mv_document_can_revert(&doc));
}

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &doc_window,
        document_pre_init, document_init, menu_actions,
        refresh_menus, mv_app_event_nop);
}
