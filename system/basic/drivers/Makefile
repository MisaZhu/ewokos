all: basic_drivers
	@echo "all done."

DIRS = nulld piped logd ramfsd timerd sdfat32fsd usbhostd hid_keybd hid_moused hid_touchd usbfat32fsd
basic_drivers: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all basic_drivers clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
