all: builtin_apps
	@echo "all done."

DIRS = splash disptest g2dtest wavplayer mp3player oggplayer load_font
builtin_apps: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all builtin_apps clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
