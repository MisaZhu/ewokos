BUILD_DIR = ../build/$(HW)
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

all: mkdirs
	cd libs; $(MAKE)
	cd drivers; $(MAKE)
	cd bin; $(MAKE)
	cd sbin; $(MAKE)
	cd xwm/opencde; $(MAKE)
	cd xwm/ewokwm; $(MAKE)
	cd apps; $(MAKE)

clean:
	cd libs; $(MAKE) clean
	cd drivers; $(MAKE) clean
	cd bin; $(MAKE) clean
	cd sbin; $(MAKE) clean
	cd xwm/opencde; $(MAKE) clean
	cd xwm/ewokwm; $(MAKE) clean
	cd apps; $(MAKE) clean
