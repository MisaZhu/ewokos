all: drivers
	@echo "all done."

DIRS = fontd displaymand consoled g2dd
drivers: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all drivers clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done