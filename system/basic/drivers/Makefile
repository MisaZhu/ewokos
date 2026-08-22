all: basic_drivers
	@echo "all done."

DIRS = nulld logd ramfsd fat32fsd timerd
basic_drivers:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = nulld logd ramfsd fat32fsd timerd
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
