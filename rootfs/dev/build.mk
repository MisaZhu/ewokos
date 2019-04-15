include dev/initfs/build.mk
include dev/sdcard/build.mk
include dev/ttyd/build.mk
include dev/consoled/build.mk
include dev/fbd/build.mk
include dev/procd/build.mk

EXTRA_CLEAN += $(PROGRAM)
