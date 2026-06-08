#ifndef _MVKIT_MV_IMAGE_H
#define _MVKIT_MV_IMAGE_H

#include <mvkit/mv_defs.h>   /* error_code */

/**
 * @file
 * @brief Loading and drawing of OS-9 graphics-buffer images.
 *
 * Images use the format produced by the repo's png-to-os9-image tool. They are
 * loaded into numbered cgfx get/put buffers, then blitted to the screen. All
 * functions return an error_code (0 on success) unless noted.
 */

/**
 * @brief Initialize the image subsystem for the calling process.
 *
 * Associates it with @p app_name, used to locate resources under
 * `/dd/SYS/<app_name>/`. The string is borrowed, not copied, so it must outlive
 * use. Frees any existing buffers. Call once before any other mv_image
 * function.
 *
 * @param app_name application name used as the resource directory.
 * @return 0 on success, or an error code if the process id could not be obtained.
 */
extern error_code mv_image_init(const char *app_name);

/**
 * @brief Load the OS-9 image file at @p path into a get/put buffer.
 * @param path          path to the OS-9 image file.
 * @param buffer_number destination get/put buffer.
 * @return 0 on success, or an error code.
 */
extern error_code mv_image_load(const char *path, int buffer_number);

/**
 * @brief Load an image from this app's resource directory.
 *
 * Loads `/dd/SYS/<app_name>/<name>` (per mv_image_init()) into @p buffer_number.
 *
 * @param name          resource file name within the app's directory.
 * @param buffer_number destination get/put buffer.
 * @return 0 on success, or an error code.
 */
extern error_code mv_image_load_resource(const char *name, int buffer_number);

/**
 * @brief Draw a loaded image to the screen with its top-left at (x, y).
 * @param buffer_number get/put buffer holding the image.
 * @param x             left edge, screen coords.
 * @param y             top edge, screen coords.
 * @return 0 on success, or an error code.
 */
extern error_code mv_image_draw(int buffer_number, int x, int y);

/**
 * @brief Free a single get/put buffer.
 * @param buffer_number the buffer to free.
 * @return 0 on success, or an error code.
 */
extern error_code mv_image_free_buffer(int buffer_number);

/**
 * @brief Free all of this process's get/put buffers.
 * @return 0 on success, or an error code.
 */
extern error_code mv_image_free_all_buffers(void);

#endif /* _MVKIT_MV_IMAGE_H */
