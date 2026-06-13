# mvdraw -- a vector drawing program for Multi-Vue, built with MVKit.
#
# Run every build inside the coco-dev toolchain image, e.g.:
#     ./coco-dev make          # build build/mvdraw.os9
#     ./coco-dev make clean
#     make run                 # launch in MAME (on the host, needs a display)
#
# This Makefile drives mvkit/app.mk (the reusable MVKit app build) and adds the
# one-time bootstrap app.mk leaves to the project: cloning + building cmoc_os9
# (libc/libcgfx) at a known-good commit and installing the vendored MVKit into
# cmoc's shared dir so the app can `#include <mvkit/mvkit.h>` and link -lmvkit.

# ---- app.mk configuration (must precede the include) ------------------------
APP          := mvdraw
SHORT        := mvd
SRCS         := drawing.c draw_view.c mvdraw.c

# Screen type 5 = 1 bpp (640x192, black & white); app.mk derives image BPP=1.
SCREEN_TYPE  := 5
WIN_W        := 80
WIN_H        := 25
WIN_BG       := 0
WIN_FG       := 1
MEM_SIZE     := 96

CMOC_OS9_DIR := cmoc_os9
BASEIMAGE    := disks/NOS9_6809_L2_v030300_coco3_80d.os9

-include mvkit/app.mk

# ---- bootstrap that app.mk leaves to the project ----------------------------
CMOC_OS9_COMMIT := 14b8f6bc983a1c694d36e3890f34b16c06a2af20

# Make the app binary wait for cmoc_os9's libc/libcgfx and an installed MVKit.
# These are order-only so they don't force a relink on every rebuild; the
# sub-makes themselves are incremental.
$(BUILD)/$(APP): | libc libcgfx mvkit

# app.mk's AIF rule has no prerequisites, so a change to WIN_W/WIN_H/SCREEN_TYPE
# would not regenerate it. Depend on this Makefile so those edits take effect.
$(AIF): Makefile

$(CMOC_OS9_DIR):
	git clone https://github.com/nitros9project/cmoc_os9.git $@
	cd $@ && git checkout $(CMOC_OS9_COMMIT)

.PHONY: libc libcgfx mvkit-install real-clean

## Build cmoc_os9's C library (libc)
libc: | $(CMOC_OS9_DIR)
	$(MAKE) -C $(CMOC_OS9_DIR)/lib all

## Build cmoc_os9's CoCo graphics library (libcgfx)
libcgfx: | $(CMOC_OS9_DIR)
	$(MAKE) -C $(CMOC_OS9_DIR)/cgfx all

## Build and install the vendored MVKit into cmoc's shared dir
mvkit:
	rm -rf xmastree
	git clone https://github.com/jamieleecho/xmastree.git
	mv xmastree/$@ .
	rm -rf xmastree
	$(MAKE) -C mvkit install

## Remove build artifacts, the installed MVKit, and the cmoc_os9 checkout
real-clean: clean
	-$(MAKE) -C mvkit uninstall
	-$(MAKE) -C mvkit clean
	rm -rf $(CMOC_OS9_DIR) mvkit xmastree

