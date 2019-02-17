include sbin/init/build.mk
include sbin/shell/build.mk
include sbin/ps/build.mk
include sbin/ls/build.mk
include sbin/pwd/build.mk
include sbin/mkdir/build.mk
include sbin/uid/build.mk
include sbin/login/build.mk

EXTRA_CLEAN += $(PROGRAM)
