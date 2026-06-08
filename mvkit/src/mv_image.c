#include <cgfx.h>
#include <fcntl.h>
#include <os.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "mvkit/mv_image.h"


static const char *my_app_name;
static char buffer[MV_PATH_MAX];
static int pid;


error_code mv_image_init(const char *app_name) {
    error_code err;
    my_app_name = app_name;
    err = _os_getpid(&pid);
    if (err) {
        return err;
    }
    return mv_image_free_all_buffers();
}


error_code mv_image_load(const char *path, int buffer_number) {
    Flush();
    int file = open(path, FAP_READ);
    int jj, err, kk = 0;
    if (file < 0) {
        return errno;
    }

    buffer[0] = 0x1b;
    buffer[1] = 0x2b;
    buffer[2] = (char)pid;
    buffer[3] = (char)buffer_number;
    if (write(MV_OUTPATH, buffer, 4) < 0) {
        err = errno;
        close(file);
        return err;
    }

    do {
        jj = read(file, buffer, sizeof(buffer));
        if (jj == 0) {
            close(file);
            return 0;
        } else if (jj < 0) {
            err = errno;
            close(file);
            return err;
        } else {
            kk = kk + jj;
            if (write(MV_OUTPATH, buffer, jj) < 0) {
                err = errno;
                close(file);
                return err;
            }
        }
    } while(true);

    return 0;
}


error_code mv_image_load_resource(const char *name, int buffer_number) {
    strcpy(buffer, "/dd/SYS/");
    strncat(buffer, my_app_name, sizeof(buffer));
    strncat(buffer, "/", sizeof(buffer));
    strncat(buffer, name, sizeof(buffer));
    buffer[MV_PATH_MAX - 1] = 0;
    return mv_image_load(buffer, buffer_number);
}


error_code mv_image_draw(int buffer_number, int x, int y) {
    return _cgfx_putblk(MV_OUTPATH, pid, buffer_number, x, y);
}


error_code mv_image_free_buffer(int buffer_number) {
    return _cgfx_kilbuf(MV_OUTPATH, pid, buffer_number);
}


error_code mv_image_free_all_buffers(void) {
    return _cgfx_kilbuf(MV_OUTPATH, pid, 0);
}
