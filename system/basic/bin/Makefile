all: builtin_apps
	@echo "all done."

DIRS = cp fan ipcserv bgrun clear echo sleep ps top devcmd svcinfo cat shell ls login sudo md5 md5sum session mount umount df uname pwd dump kill killall rm chmod chown chgrp whoami mkdir mkfifo grep head more rx elfinfo sysinfo mmio date vi test malloc4g concat dltest find wc file
#	test
# apps are independent of each other, build them in parallel under -j
builtin_apps: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all builtin_apps clean $(DIRS)

#	test
clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
