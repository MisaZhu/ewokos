BUILD_DIR = ../build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..


DIRS = libs drivers bin sbin xwm/opencde xwm/ewokwm apps
all: mkdirs $(DIRS)
	@echo "all done."

# everything else links against the x libs
drivers bin sbin xwm/opencde xwm/ewokwm apps: libs
$(DIRS): mkdirs
	@$(MAKE) -C $@

.PHONY: all mkdirs clean $(DIRS)

mkdirs:
	@mkdir -p ${TARGET_DIR}/drivers
	@mkdir -p ${TARGET_DIR}/apps
	@mkdir -p ${TARGET_DIR}/sbin/x
	@mkdir -p ${TARGET_DIR}/bin/x
	@mkdir -p ${TARGET_DIR}/data ${TARGET_DIR}/usr ${TARGET_DIR}/etc
	@cp -Rp data/. ${TARGET_DIR}/data/
	@cp -Rp usr/. ${TARGET_DIR}/usr/
	@cp -Rp etc/. ${TARGET_DIR}/etc/
	@mkdir -p ${TARGET_DIR}/usr/x/themes/default
	@cp -Rp usr/x/themes/opencde/. ${TARGET_DIR}/usr/x/themes/default/

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
