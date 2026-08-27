all: drivers
	@echo "all done."

DIRS = xserverd vjoystickd vkeybd
drivers: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all drivers clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done