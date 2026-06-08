# Shared settings for the in-repo "Using MVKit" guide examples.
#
# These point app.mk at the repo's vendored cmoc_os9 checkout and the NitrOS-9
# base disk -- things a standalone app would instead have installed or keep in
# its own project. Each example carries its own assets/ (like a real app), so
# app.mk's default ASSETS=assets applies. A standalone app's Makefile would just
# set APP/SRCS and `include /usr/local/share/cmoc/app.mk`.
#
# Paths are relative to an example directory (mvkit/guide/NN-*/), which is where
# make runs.
CMOC_OS9_DIR ?= ../../../cmoc_os9
BASEIMAGE    ?= ../../../disks/NOS9_6809_L2_v030300_coco3_80d.os9

include ../../app.mk
