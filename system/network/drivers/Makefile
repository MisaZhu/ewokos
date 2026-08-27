all: basic_drivers
	@echo "all done."

DIRS = netd timed
basic_drivers: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all basic_drivers clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
