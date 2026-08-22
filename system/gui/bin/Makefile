all: builtin_apps
	@echo "all done."

DIRS = splash disptest g2dtest wavplayer mp3player oggplayer load_font
builtin_apps:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = splash disptest g2dtest wavplayer mp3player oggplayer load_font
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
