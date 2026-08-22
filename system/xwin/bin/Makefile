DIRS = xlauncher statusbar xsession xfont anim wlayout
all:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = xlauncher statusbar xsession xfont anim wlayout
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
