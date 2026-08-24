all: basic_drivers
	@echo "all done."

DIRS = nulld logd ramfsd timerd sdfat32fsd usbfat32fsd
basic_drivers:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
