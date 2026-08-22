all: drivers
	@echo "all done."

DIRS = xserverd vjoystickd vkeybd
drivers:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = xserverd vjoystickd vkeybd
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done