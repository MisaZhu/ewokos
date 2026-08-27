all: builtin_apps
	@echo "all done."

DIRS = cp ipcserv bgrun tsaver clear echo sleep ps devcmd svcinfo cat shell ls login md5 session mount umount df uname pwd dump kill rm chmod chown chgrp whoami mkdir mkfifo grep head more rx elfinfo json sysinfo mmio date vi stltest
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
