all: builtin_apps
	@echo "all done."

DIRS = cp ipcserv bgrun tsaver clear echo sleep ps devcmd svcinfo cat shell ls login md5 session mount df uname pwd dump kill rm chmod chown chgrp whoami mkdir mkfifo grep head more rx elfinfo json sysinfo mmio date vi stltest
#	test
builtin_apps:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = cp ipcserv bgrun tsaver clear echo sleep ps devcmd svcinfo cat shell ls login md5 session mount df uname pwd dump kill rm chmod chown chgrp mkdir mkfifo grep head more rx elfinfo whoami json sysinfo mmio date vi stltest
#	test
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
