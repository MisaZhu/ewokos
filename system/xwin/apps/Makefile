DIRS = xlog xterm xread xcores xfonts xapps cards mine xfinder ximg xprocs xDemo wDemo xwm_theme xtheme xwifi clock xg2dtest SndPlayer
all: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
