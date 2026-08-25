BUILD_DIR = ../build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..


all: mkdirs
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/drivers
	@mkdir -p ${TARGET_DIR}/apps
	@mkdir -p ${TARGET_DIR}/sbin/x
	@mkdir -p ${TARGET_DIR}/bin/x
	@cp -r data ${TARGET_DIR}
	@cp -r usr ${TARGET_DIR}
	@cp -r etc ${TARGET_DIR}
	@mkdir -p ${TARGET_DIR}/usr/x/themes/default
	@cp -r usr/x/themes/opencde/* ${TARGET_DIR}/usr/x/themes/default/

DIRS = libs drivers bin sbin xwm/opencde xwm/ewokwm apps
all: mkdirs
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = libs drivers bin sbin xwm/opencde xwm/ewokwm apps
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
