include sbin/init/build.mk
include sbin/shell/build.mk
include sbin/userman/build.mk
include sbin/vfs/build.mk
include sbin/ps/build.mk
include sbin/ls/build.mk
include sbin/pwd/build.mk
include sbin/mkdir/build.mk
include sbin/uid/build.mk
include sbin/test/build.mk

EXTRA_CLEAN += $(PROGRAM)
