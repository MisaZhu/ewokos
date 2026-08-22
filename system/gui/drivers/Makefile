all: drivers
	@echo "all done."

DIRS = fontd displaymand consoled
drivers:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = fontd displaymand consoled
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done