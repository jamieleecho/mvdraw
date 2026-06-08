/*
 * mv_file_dialog() is MVKit's Save/Open file dialog, cloned from cmoc_os9's
 * cgfx MVFName() and customized with Save/Open + Cancel buttons. `confirm_label`
 * is the text on the confirm button ("Open"/"Save"). Returns a pointer to the
 * chosen filename, or NULL if cancelled.
 *
 * The box has a WT_DBOX border and a centered title, with a scrolling file list
 * and a scrollbar (divider + '^'/'v' arrows) on the right; the scrollbar drag is
 * modal.
 */

#include <cgfx.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include <mvkit/mv_defs.h>
#include "mvkit/mv_file_dialog.h"

#define FILE_DIALOG_BOX_WIDTH    24   /* content (22) + WT_DBOX border */
#define FILE_DIALOG_BOX_HEIGHT1  14   /* content (12) + WT_DBOX border */
#define FILE_DIALOG_BUTTON_ROW   10
#define FILE_DIALOG_CONFIRM_COL  2
#define FILE_DIALOG_CANCEL_COL   12
#define FILE_DIALOG_BUTTON_WIDTH 6    /* uniform button width (== strlen("Cancel")) */
#define FILE_DIALOG_MAX_FILES    128  /* cap on listed entries (keeps files[] small) */

static char _FName[30];
static int files[FILE_DIALOG_MAX_FILES];
static char dbuf[32];
static MSRET mp;
static struct sgbuf oldopts, newopts;

long _gs_pos(path_id path);
int getstr(int path, const char *title, char *s, int n, int column, int row, int fg, int bg);

/* True if `s` ends with `ext`, which includes its leading dot (e.g. ".xmt"). */
static int mv_has_ext(const char *s, const char *ext)
{
    int slen = strlen(s);
    int elen = strlen(ext);

    return ((slen >= elen) && (strcmp(s + slen - elen, ext) == 0));
}

/* True if `name` can be opened as a directory (the cheap way to tell, since an
   OS-9 directory entry doesn't carry the dir attribute). */
static int mv_is_dir(const char *name)
{
    int p = open(name, FAM_READ | S_DIR);

    if (p == -1)
        return 0;
    close(p);
    return 1;
}

/* If `ext` is non-NULL/non-empty and `s` does not already end with it, append
   `ext` (which includes its dot, e.g. ".xmt"), provided `ext` and the NUL still
   fit in a buffer of `size` bytes. */
static void mv_append_ext(char *s, int size, const char *ext)
{
    int slen, elen;

    if (!ext || !*ext || mv_has_ext(s, ext))
        return;
    slen = strlen(s);
    elen = strlen(ext);
    if (slen + elen + 1 > size)
        return;                         /* ext + NUL would not fit */
    strcpy(s + slen, ext);
}

static void trim(char *s) {
    char *p = s;
    int l = strlen(p);

    // Trim trailing whitespace
    while(l > 0 && isspace(p[l-1])) p[--l] = 0;

    // Trim leading whitespace
    while(*p && isspace(*p)) ++p, --l;

    // Shift remaining string to the front
    strcpy(s, p);
}

/* ---- Working-directory tracker -------------------------------------------
   The dialog chdir()s while browsing and must restore the starting CWD on
   cancel, but OS-9 has no cheap getcwd. So we track navigation instead:
   `level` counts directories descended below the start (undo with ".."), and
   once we climb above the start, `buffer` holds the "name/name/..." path to
   descend back down. On overflow or any failure, level is set to -1 and every
   op becomes a no-op (we silently give up on restoring). */
typedef struct {
    int level;
    char buffer[128];
} CwdTracker;

static CwdTracker cwd_tracker;

/* Name of the current directory, found the OS-9 way (cf. the `pd` command):
   the "." entry holds this dir's own descriptor LSN, and the entry with the
   matching LSN in ".." carries its name. Entries are matched by name (not
   position -- OS-9 lists ".." before "."). Returns 1 with `out` set on success,
   -1 if we're at the device root ("." and ".." share an LSN; chdir("..") there
   is a no-op), or 0 if the name can't be determined. */
static int cwd_current_name(char *out)
{
    int cur, par, found = 0, have_self = 0, have_parent = 0;
    char entry[32];
    char name[30];
    char self[3], parent[3];

    /* One pass over the current dir gets both its own LSN ("." entry) and its
       parent's ("..") -- enough to also detect the root. */
    cur = open(".", FAM_READ | S_DIR);
    if (cur == -1)
        return 0;
    while ((!have_self || !have_parent) && read(cur, entry, 32) == 32)
    {
        if (entry[0] == 0)
            continue;
        strhcpy(name, entry);
        if (strcmp(name, ".") == 0)
        {
            self[0] = entry[29]; self[1] = entry[30]; self[2] = entry[31];
            have_self = 1;
        }
        else if (strcmp(name, "..") == 0)
        {
            parent[0] = entry[29]; parent[1] = entry[30]; parent[2] = entry[31];
            have_parent = 1;
        }
    }
    close(cur);
    if (!have_self || !have_parent)
        return 0;
    if (self[0] == parent[0] && self[1] == parent[1] && self[2] == parent[2])
        return -1;                      /* at the device root */

    /* Scan the parent for the entry with our LSN; its name is ours. */
    par = open("..", FAM_READ | S_DIR);
    if (par == -1)
        return 0;
    while (read(par, entry, 32) == 32)
    {
        if (entry[0] == 0)
            continue;
        if (entry[29] != self[0] || entry[30] != self[1] || entry[31] != self[2])
            continue;
        strhcpy(name, entry);
        if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
        {
            strcpy(out, name);
            found = 1;
            break;
        }
    }
    close(par);
    return found;
}

static void cwd_init(CwdTracker *c)
{
    c->level = 0;
    c->buffer[0] = 0;
}

/* Index of the first char of the buffer's last "name/" segment (buffer is
   non-empty and ends with '/'). */
static int cwd_last_seg(const char *buf)
{
    int seg = strlen(buf) - 1;          /* the trailing '/' */

    while (seg > 0 && buf[seg - 1] != '/')
        seg--;
    return seg;
}

static void cwd_down(CwdTracker *c, const char *child)
{
    int seg, slen;

    if (c->level < 0)
        return;
    if (c->buffer[0] == 0)
    {
        c->level++;                     /* descending below the start dir */
        return;
    }
    /* Above the start dir: expect to descend back along the recorded path. */
    seg = cwd_last_seg(c->buffer);
    slen = strlen(c->buffer) - 1 - seg;
    if ((int) strlen(child) == slen && strncmp(child, c->buffer + seg, slen) == 0)
        c->buffer[seg] = 0;             /* popped one step back toward start */
    else
        c->level = c->level + 1;        /* pushed further down */
}

static void cwd_up(CwdTracker *c)
{
    char name[30];
    int blen, nlen, r;

    if (c->level < 0)
        return;
    if (c->level > 0)
    {
        c->level--;                     /* climbing back toward the start dir */
        return;
    }
    /* Climbing above the start dir: record the folder we're leaving (must run
       while still in it) so restore can descend back into it. */
    r = cwd_current_name(name);
    if (r < 0)
        return;                         /* at the root: chdir("..") is a no-op */
    if (r == 0)
    {
        c->level = -1;                  /* name unknown: give up silently */
        return;
    }
    nlen = strlen(name);
    blen = strlen(c->buffer);
    if (blen + nlen + 2 > (int) sizeof(c->buffer))
    {
        c->level = -1;                  /* would overflow: give up silently */
        return;
    }
    strcpy(c->buffer + blen, name);
    c->buffer[blen + nlen] = '/';
    c->buffer[blen + nlen + 1] = 0;
}

static void cwd_restore(CwdTracker *c)
{
    int seg;

    if (c->level < 0)
        return;                         /* gave up earlier: leave CWD as-is */
    while (c->level > 0)
    {
        chdir("..");
        c->level--;
    }
    while (c->buffer[0])                 /* descend the recorded path, last first */
    {
        seg = cwd_last_seg(c->buffer);
        c->buffer[strlen(c->buffer) - 1] = 0;   /* drop trailing '/' for chdir */
        chdir(c->buffer + seg);
        c->buffer[seg] = 0;             /* pop the segment */
    }
}

/* Load list entry `index` into _FName (-1 = the "[new file]" slot). */
static void load_name(int dpath, int index)
{
    if (index == -1)
        strcpy(_FName, "[new file]");
    else
    {
        lseek(dpath, (long) files[index], 0);
        read(dpath, dbuf, 32);
        strhcpy(_FName, dbuf);
    }
}

/* Read the mouse, converting its window pixel coords to 8x8 character cells so
   the rest of the dialog works purely in character coordinates. */
static void read_mouse(int path)
{
    _cgfx_gs_mouse(path, &mp);
    mp.pt_wrx /= 8;
    mp.pt_wry /= 8;
}

char *mv_file_dialog(path_id path, const char *title, const char *confirm_label,
                      int allow_new, const char *ext,
                      int column, int row, int fg, int bg)
{
    int dpath, temp;
    int line, startnum, refresh, index;
    int numfiles;
    char ch;
    int scrlsize, ypos;
    int bflag, dragging;
    int minidx;   /* lowest list index: -1 = the [new file] slot, 0 = files only */
    char cf[FILE_DIALOG_BUTTON_WIDTH + 1];   /* confirm-button label (loop reuses it) */

    minidx = allow_new ? -1 : 0;
    cwd_init(&cwd_tracker);

    _gs_opt(path, &oldopts);
    _gs_opt(path, &newopts);
    newopts.sg_echo = 0;
    newopts.sg_kbich = 0;
    newopts.sg_kbach = 0;
    _ss_opt(path, &newopts);

    /* Drain queued keystrokes so a stray key can't immediately act on the
       dialog the instant it appears. */
    while (_gs_rdy(path) != -1)
        read(path, &ch, 1);

    /* Box is 2 chars larger than the content on each axis. Set the font before
       ss_wnset so the WT_DBOX frame is sized in 8x8 cells; ss_wnset draws the
       frame and insets the drawing area by one char. */
    _cgfx_owset(path, 1, column, row, FILE_DIALOG_BOX_WIDTH, FILE_DIALOG_BOX_HEIGHT1, fg, bg);
    _cgfx_curoff(path);
    _cgfx_setgc(path, GRP_PTR, PTR_ARR);
    _cgfx_scalesw(path, 0);
    _cgfx_tcharsw(path, 0);
    _cgfx_font(path, GRP_FONT, FNT_S8X8);
    _cgfx_ss_wnset(path, WT_DBOX, 0);

    /* Scrollbar divider: vertical line at x=168, y=16..79. */
    _cgfx_setdptr(path, 168, 16);
    _cgfx_line(path, 168, 79);

    /* Scroll arrows, drawn once. Rows 1 and 9 match the up/down click targets. */
    _cgfx_curxy(path, 21, 1);
    cwrite(path, "^", 1);
    _cgfx_curxy(path, 21, 9);
    cwrite(path, "v", 1);

    /* Title, centered on the top interior row. */
    {
        int tlen = strlen(title);
        int tcol = (FILE_DIALOG_BOX_WIDTH - 2 - tlen) / 2;
        if (tcol < 0)
            tcol = 0;
        _cgfx_curxy(path, tcol, 0);
        cwrite(path, title, tlen);
    }
    /* +2 (not +1): the list-clear overlays below are not WT_DBOX-inset, so they
       need the extra char to land below the inset title row, not over it. */
    column += 2;
    row += 2;
    bflag = 0;
    dragging = 0;

    /* Confirm (Open/Save) and Cancel buttons along the bottom; the confirm
       label is centered in a fixed-width field so both are the same size. */
    {
        int bl = strlen(confirm_label);
        int bp = (FILE_DIALOG_BUTTON_WIDTH - bl) / 2;
        int bi;
        if (bp < 0)
            bp = 0;
        for (bi = 0; bi < FILE_DIALOG_BUTTON_WIDTH; bi++)
            cf[bi] = (bi >= bp && bi < bp + bl) ? confirm_label[bi - bp] : ' ';
        cf[FILE_DIALOG_BUTTON_WIDTH] = 0;
        BUp(path, FILE_DIALOG_CONFIRM_COL, FILE_DIALOG_BUTTON_ROW, cf, fg, bg);
    }
    BUp(path, FILE_DIALOG_CANCEL_COL, FILE_DIALOG_BUTTON_ROW, "Cancel", fg, bg);
    Flush();

    while (1)
    {
        dpath = open(".", FAM_READ | S_DIR);
        if (dpath == -1)
        {
            _cgfx_owend(path);
            _ss_opt(path, &oldopts);
            return 0;
        }

        index = 0;
        numfiles = -1;
        while (index < FILE_DIALOG_MAX_FILES)
        {
            if (read(dpath, dbuf, 32) < 32)
            {
                numfiles = index - 1;
                break;
            }
            if (dbuf[0] == 0)
                continue;               /* unused/erased directory slot */
            /* With an extension filter, list only matching files plus any
               directory (so navigation still works); otherwise list all. */
            if (ext)
            {
                strhcpy(_FName, dbuf);
                if (!mv_has_ext(_FName, ext) && !mv_is_dir(_FName))
                    continue;
            }
            files[index++] = (int) (_gs_pos(dpath) - 32L);
        }

        refresh = 1;
        scrlsize = 53 / (numfiles / 8 + 1);
        if (scrlsize < 4)
            scrlsize = 4;

        startnum = minidx;
        line = 0;

        while (1)
        {
            if (refresh)
            {
                _cgfx_owset(path, 0, column, row, 20, 8, fg, bg);
                _cgfx_clear(path);
                Flush();
                _cgfx_owend(path);

                for (index = startnum; index < startnum + 8; index++)
                    if (index <= numfiles)
                    {
                        load_name(dpath, index);
                        _cgfx_curxy(path, 1, index - startnum + 1);
                        if (index == (startnum + line))
                            _cgfx_revon(path);
                        else
                            _cgfx_revoff(path);
                        cwrite(path, _FName, strlen(_FName));
                    }
                refresh = 0;
            }

            _cgfx_fcolor(path, bg);
            _cgfx_setdptr(path, 170, 17);
            _cgfx_bar(path, 173, 71);
            temp = 17 + (startnum + line) * (53 - scrlsize) / numfiles;
            if (temp < 17)
                temp = 17;
            if (temp > 17 + 53 - scrlsize)   /* keep the thumb inside the track */
                temp = 17 + 53 - scrlsize;
            _cgfx_setdptr(path, 170, temp);
            _cgfx_fcolor(path, fg);
            _cgfx_rbar(path, 3, scrlsize);

            load_name(dpath, startnum + line);
            _cgfx_curxy(path, 1, line + 1);
            _cgfx_revon(path);
            cwrite(path, _FName, strlen(_FName));
            _cgfx_revoff(path);
            Flush();

            do
            {
                read_mouse(path);
                if ((_gs_rdy(path) == -1) && !mp.pt_cbsa)
                {
                    bflag = 0;
                    dragging = 0;   /* button released: end any scrollbar drag */
                }
            } while ((_gs_rdy(path) == -1) && !(mp.pt_cbsa ^ bflag));

            _cgfx_curxy(path, 1, line + 1);
            cwrite(path, _FName, strlen(_FName));
            Flush();

            if (_gs_rdy(path) == -1)
                ch = 0;
            else
                read(path, &ch, 1);
            ypos = mp.pt_wry;

            if (!mp.pt_valid && ch == 0)
                continue;
            if (ch)
                mp.pt_wrx = ypos = -1;

            /* Modal scrollbar drag: once a press lands in the track, keep
               scrolling until release -- skip the buttons/file rows so a drag
               can't trigger Open/Save/Cancel. startnum is clamped so the thumb
               stays within the track. */
            if (ch == 0 && mp.pt_cbsa &&
                (dragging || ((mp.pt_wrx == 21) && (ypos > 1) && (ypos < 9))))
            {
                int ns = (mp.pt_wry - 2) * numfiles / 7;
                if (ns < minidx)
                    ns = minidx;
                if (ns > numfiles)
                    ns = numfiles;
                dragging = 1;
                if (ns != startnum)
                {
                    startnum = ns;
                    line = 0;
                    refresh = 1;
                }
                continue;
            }

            /* Modal button press. While the fire button is held, show the
               pressed button down when the mouse is inside it and up when it
               leaves; nothing else on screen is touched. Only act on release
               with the mouse still inside (confirm = Enter, Cancel = Esc). */
            if (ch == 0 && mp.pt_cbsa && ypos == FILE_DIALOG_BUTTON_ROW)
            {
                int bcol, confirm, down, inbtn;
                const char *blabel;

                if (mp.pt_wrx >= FILE_DIALOG_CONFIRM_COL &&
                    mp.pt_wrx < FILE_DIALOG_CONFIRM_COL + FILE_DIALOG_BUTTON_WIDTH)
                {
                    bcol = FILE_DIALOG_CONFIRM_COL;
                    confirm = 1;
                    blabel = cf;
                }
                else if (mp.pt_wrx >= FILE_DIALOG_CANCEL_COL &&
                         mp.pt_wrx < FILE_DIALOG_CANCEL_COL + FILE_DIALOG_BUTTON_WIDTH)
                {
                    bcol = FILE_DIALOG_CANCEL_COL;
                    confirm = 0;
                    blabel = "Cancel";
                }
                else
                    continue;   /* button row, but between/outside the buttons */

                BDown(path, bcol, FILE_DIALOG_BUTTON_ROW, blabel);
                down = 1;
                do
                {
                    read_mouse(path);
                    inbtn = (mp.pt_wry == FILE_DIALOG_BUTTON_ROW) &&
                            (mp.pt_wrx >= bcol) &&
                            (mp.pt_wrx < bcol + FILE_DIALOG_BUTTON_WIDTH);
                    if (inbtn && !down)
                    {
                        BDown(path, bcol, FILE_DIALOG_BUTTON_ROW, blabel);
                        down = 1;
                    }
                    else if (!inbtn && down)
                    {
                        BUp(path, bcol, FILE_DIALOG_BUTTON_ROW, blabel, fg, bg);
                        down = 0;
                    }
                } while (mp.pt_cbsa);

                if (!down)
                    continue;   /* released outside the button: do nothing */

                BUp(path, bcol, FILE_DIALOG_BUTTON_ROW, blabel, fg, bg);
                ch = confirm ? 13 : 5;
                mp.pt_wrx = ypos = -1;
            }

            if ((((mp.pt_wrx == 21) && (ypos == 9)) || (ch == 10)) && ((line + startnum) < numfiles))
            {
                line++;
                if (line > 7)
                {
                    _cgfx_owset(path, 0, column, row, 20, 8, fg, bg);
                    _cgfx_delline(path);
                    Flush();
                    _cgfx_owend(path);
                    line = 7;
                    startnum++;
                }
            }
            else if ((((mp.pt_wrx == 21) && (ypos == 1)) || (ch == 12)) && ((line + startnum) > minidx))
            {
                line--;
                if (line < 0)
                {
                    _cgfx_owset(path, 0, column, row, 20, 8, fg, bg);
                    _cgfx_insline(path);
                    Flush();
                    _cgfx_owend(path);
                    startnum--;
                    line = 0;
                }
            }
            else if (((ypos > 0) && (ypos < 9) && (mp.pt_wrx < 21)) || (ch == 13))
            {
                if ((ypos == (line + 1)) || (ch == 13))
                {
                    if ((startnum + line) == -1) {
                        getstr(path, "Filename? ", _FName, 16, column - 2, row + 4, fg, bg);
                        _FName[16] = 0;
                        trim(_FName);
                        if (strlen(_FName) <= 0) {
                            continue;   /* dpath still open: keep browsing */
                        }
                    }

                    /* Going up: record the folder we're leaving (while we're
                       still in it) before chdir("..") so cancel can restore. */
                    if (strcmp(_FName, "..") == 0)
                    {
                        close(dpath);
                        cwd_up(&cwd_tracker);
                        chdir("..");
                        break;
                    }

                    close(dpath);

                    if (chdir(_FName) == -1)
                    {
                        /* It's a file (or a typed new name), not a directory to
                           enter. Save (allow_new) normalizes the name to carry
                           the extension; Open returns the pick as-is. */
                        if (allow_new)
                            mv_append_ext(_FName, sizeof(_FName), ext);
                        _cgfx_owend(path);
                        _ss_opt(path, &oldopts);
                        return _FName;
                    }

                    /* Navigated into a subdirectory. */
                    if (strcmp(_FName, ".") != 0)
                        cwd_down(&cwd_tracker, _FName);
                    break;
                }
                else if ((startnum + ypos - 1) <= numfiles)
                {
                    line = ypos - 1;
                    bflag = 1;
                }
            }
            else if (ch == 5)
            {
                cwd_restore(&cwd_tracker);
                _cgfx_owend(path);
                _ss_opt(path, &oldopts);
                close(dpath);
                return 0;
            }
        }
    }
}
