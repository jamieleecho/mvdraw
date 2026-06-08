# Makefile to build the MVKit static library (libmvkit.a).
#
# MVKit is the reusable, app-agnostic core extracted from the xmastree example
# app. It links against cmoc_os9's libc/libcgfx exactly as the app does. The
# layout (include/mvkit + src) mirrors cgfx so the eventual lift into cmoc_os9
# is a near-verbatim copy.

CC := cmoc
AR := lwar
CMOC_OS9_DIR := ../cmoc_os9
CFLAGS := --os9 -Iinclude -I${CMOC_OS9_DIR}/include -I${CMOC_OS9_DIR}/cgfx/include

LIB := libmvkit.a

# Install location. cmoc auto-searches PREFIX/include and PREFIX/lib, so an app
# that links an installed MVKit needs no -I/-L for it -- just `-lmvkit` and
# `#include <mvkit/mvkit.h>`. Override PREFIX (and DESTDIR for staged/packaged
# installs) as needed.
PREFIX ?= /usr/local/share/cmoc

# Doxygen generates the HTML API docs (see Doxyfile). It ships in the coco-dev
# image (>= 0.83), alongside the cmoc toolchain, so `make docs` just runs it.
# Override to point at a different binary if needed: make docs DOXYGEN=/path/doxygen
DOXYGEN ?= doxygen

# Object modules that make up the library. Modules are added here as they are
# migrated out of xmastree.
MODS := src/mv_version.o src/mv_image.o src/mv_file_dialog.o \
        src/mv_undo_manager_init.o src/mv_undo_manager_push_undo.o \
        src/mv_undo_manager_can_undo.o src/mv_undo_manager_undo.o \
        src/mv_undo_manager_all_undone.o src/mv_undo_manager_reset_undo_marker.o \
        src/mv_undo_manager_reset.o \
        src/mv_app_init.o src/mv_app_run.o src/mv_app_message_box.o src/mv_app_dialog.o \
        src/mv_app_nop.o \
        src/mv_document_ensure_extension.o src/mv_document_save_internal.o \
        src/mv_document_init.o src/mv_document_new.o src/mv_document_open.o \
        src/mv_document_opened.o src/mv_document_revert.o src/mv_document_save_as.o \
        src/mv_document_save.o src/mv_document_make_change.o src/mv_document_undo.o \
        src/mv_document_queries.o \
        src/mv_view.o src/mv_image_grid.o \
        src/mv_theme_default.o src/mv_app_set_theme.o src/mv_theme_chrome_ordered.o

.PHONY: all clean docs guide help install uninstall

# The Using-MVKit guide example apps (mvkit/guide/NN-*).
GUIDE_EXAMPLES := $(wildcard guide/[0-9]*)

## Build the MVKit static library (default target)
all: ${LIB}

${LIB}: ${MODS}
	${AR} -c $@ ${MODS}

src/%.o: src/%.c
	${CC} ${CFLAGS} -c -o $@ $<

## Install headers and the library into PREFIX (cmoc's shared dir by default)
install: all
	mkdir -p ${DESTDIR}${PREFIX}/include/mvkit ${DESTDIR}${PREFIX}/lib
	cp include/mvkit/*.h ${DESTDIR}${PREFIX}/include/mvkit/
	cp include/stdbool.h ${DESTDIR}${PREFIX}/include/
	cp ${LIB} ${DESTDIR}${PREFIX}/lib/
	cp app.mk ${DESTDIR}${PREFIX}/

## Remove the headers and library installed by `install`
uninstall:
	@rm -rf ${DESTDIR}${PREFIX}/include/mvkit
	@rm -f ${DESTDIR}${PREFIX}/include/stdbool.h
	@rm -f ${DESTDIR}${PREFIX}/lib/${LIB}
	@rm -f ${DESTDIR}${PREFIX}/app.mk

## Generate the HTML API documentation into docs/ (uses Doxygen; see DOXYGEN)
docs:
	${DOXYGEN} Doxyfile

## Build the Using MVKit guide example apps (mvkit/guide/NN-*)
# Needs cmoc_os9 and an installed MVKit; from a clean tree run the top-level
# `make mvkit-guide`, which builds those first.
guide:
	@for d in ${GUIDE_EXAMPLES}; do echo "==> $$d"; ${MAKE} -C $$d || exit 1; done

## Remove build artifacts, generated docs, and each guide example's build
clean:
	@rm -f src/*.o ${LIB}
	@rm -rf docs
	@for d in ${GUIDE_EXAMPLES}; do if [ -f "$$d/Makefile" ]; then ${MAKE} --no-print-directory -C $$d clean; fi; done

## Show this help message
help:
	@awk 'BEGIN { FS = ":"; printf "Usage: make <target>\n\nTargets:\n"; } \
		/^## / { doc = substr($$0, 4); next } \
		/^[a-zA-Z_][a-zA-Z0-9_-]*:/ { if (doc) printf "  \033[36m%-16s\033[0m %s\n", $$1, doc; doc = ""; next } \
		{ doc = "" }' $(MAKEFILE_LIST)
