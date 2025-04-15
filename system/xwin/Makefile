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
	cd libs; make
	cd drivers; make
	cd bin; make
	cd sbin; make
	cd xwm/opencde; make
	cd apps; make

clean:
	cd libs; make clean
	cd drivers; make clean
	cd bin; make clean
	cd sbin; make clean
	cd xwm/opencde; make clean
	cd apps; make clean
