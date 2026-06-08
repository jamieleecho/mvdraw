/*
 * Using MVKit, stage 7: undo.
 *
 * Stage 6 already recorded how to reverse every change (the MVUndoItem passed to
 * mv_document_make_change). This stage spends that: an Edit > Undo menu calls
 * mv_document_undo, which pops the most recent change and runs its undo function
 * -- here, putting the counter back. The document's undo manager also tracks the
 * dirty state, so undoing back to the last saved point clears "modified".
 *
 * The only additions over stage 6 are the Edit menu, undo_action, and enabling
 * Undo from mv_document_can_undo in refresh_menus.
 */
#include <fcntl.h>      /* open, creat, FAP_READ, FAP_WRITE */
#include <stdio.h>
#include <unistd.h>     /* read, write, close */
#include <errno.h>

#include <mvkit/mvkit.h>

#define MN_COUNT 30     /* app-chosen menu id (cgfx reserves the low numbers) */

typedef struct {
    int count;
} Counter;

static Counter counter;
static MVDocument doc;

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

/* Reverse one Increment. mv_document_undo calls this with the object recorded
   alongside it (here, the counter). */
static void counter_undo_increment(void *object) {
    ((Counter *)object)->count -= 1;
}

typedef enum {
    FileIndex_Save   = 2,
    FileIndex_Revert = 4,
} FileIndex;

typedef enum {
    EditIndex_Undo = 0,
} EditIndex;

static MIDSCR file_items[] = {
    MV_MENU_ITEM("New"),          /* item 1 */
    MV_MENU_ITEM("Open..."),      /* item 2 */
    MV_MENU_ITEM("Save"),         /* item 3 */
    MV_MENU_ITEM("Save As..."),   /* item 4 */
    MV_MENU_ITEM("Revert"),       /* item 5 */
};

static MIDSCR edit_items[] = {
    MV_MENU_ITEM("Undo"),         /* item 1 */
};

static MIDSCR count_items[] = {
    MV_MENU_ITEM("Increment"),    /* item 1 */
};

static MNDSCR menus[] = {
    MV_MENU("File", MN_FILE, file_items),
    MV_MENU("Edit", MN_EDIT, edit_items),
    MV_MENU("Count", MN_COUNT, count_items),
};

mv_set_menus(undo_window, "undo", menus);


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

static void undo_action(MSRET *msinfo, int menuid, int itemno) {
    if (mv_document_undo(&doc)) {
        show();
    }
}

static void increment_action(MSRET *msinfo, int menuid, int itemno) {
    MVUndoItem undo = { counter_undo_increment, &counter };
    counter.count += 1;
    mv_document_make_change(&doc, &undo);   /* marks dirty + records the undo */
    show();
}

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
    {MN_EDIT, 1, undo_action},
    {MN_COUNT, 1, increment_action},
    {MN_CLOS, 1, close_action},
    MV_MENU_ACTION_END
};


static void undo_pre_init(int argc, char **argv) {
    mv_document_init(&doc, NULL, "counter", ".cnt", &counter,
                     counter_new, counter_open, counter_save);
    if (argc == 2) {
        counter_open(&counter, argv[1]);
        mv_document_opened(&doc);
    }
}

static void undo_init(void) {
    printf("Count > Increment, then Edit > Undo to take it back.\n");
    show();
}

static void refresh_menus(void) {
    mv_menu_item_set_enabled(file_items, FileIndex_Save,   mv_document_is_dirty(&doc));
    mv_menu_item_set_enabled(file_items, FileIndex_Revert, mv_document_can_revert(&doc));
    mv_menu_item_set_enabled(edit_items, EditIndex_Undo,   mv_document_can_undo(&doc));
}

int main(int argc, char **argv) {
    return mv_app_run(argc, argv, &undo_window,
        undo_pre_init, undo_init, menu_actions,
        refresh_menus, mv_app_event_nop);
}
