DIRS = xlog xterm xread xcores xfonts xapps cards mine xfinder ximg xprocs xDemo wDemo xwm_theme xtheme xwifi clock xg2dtest SndPlayer
all:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = xlog xterm xread xcores xfonts xapps cards mine xfinder ximg xprocs xDemo wDemo xwm_theme xtheme xwifi clock xg2dtest SndPlayer
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
