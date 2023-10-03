BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..

all: mkdirs bins extra_libs extra_driver xapps
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/apps
	@mkdir -p ${TARGET_DIR}/sbin/x
	@mkdir -p ${TARGET_DIR}/bin/x
	@cp -r data ${TARGET_DIR}
	@cp -r etc ${TARGET_DIR}

extra_libs:
	cd libs; make

bins:
	cd bin; make

xapps:
	cd x; make
	cd apps; make

extra_driver:
	cd drivers; make

clean:
	cd libs; make clean
	cd bin; make clean
	cd drivers; make clean
	cd x; make clean
	cd apps; make clean
