BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..

all: mkdirs extra_libs bins extra_driver xapps
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/apps
	@mkdir -p ${TARGET_DIR}/sbin/x
	@mkdir -p ${TARGET_DIR}/bin/x
	@cp -r data ${TARGET_DIR}
	@cp -r usr ${TARGET_DIR}
	@cp -r etc ${TARGET_DIR}
	@mkdir -p ${TARGET_DIR}/usr/x/themes/default
	@cp -r usr/x/themes/opencde/* ${TARGET_DIR}/usr/x/themes/default/

extra_libs:
	cd libs; make

bins:
	cd bin; make
	cd sbin; make

xapps:
	cd x; make
	cd apps; make

extra_driver:
	cd drivers; make

clean:
	cd libs; make clean
	cd bin; make clean
	cd sbin; make clean
	cd drivers; make clean
	cd x; make clean
	cd apps; make clean
