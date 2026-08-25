all: basic_drivers
	@echo "all done."

DIRS = nulld logd ramfsd timerd sdfat32fsd usbhostd hid_keybd hid_moused hid_touchd usbfat32fsd
basic_drivers:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
