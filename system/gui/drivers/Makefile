all: drivers
	@echo "all done."

DIRS = fontd displaymand consoled g2dd usbhostd usbfat32fsd btd hid_keybd hid_moused hid_touchd
drivers: $(DIRS)

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all drivers clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done