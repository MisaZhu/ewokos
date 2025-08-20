BUILD_DIR = ../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..


all: mkdirs extra_libs bins sbins extra_driver 
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/drivers
	@cp -r usr ${TARGET_DIR}
	@cp -r etc ${TARGET_DIR}

extra_libs:
	cd libs; make

bins:
	cd bin; make

sbins:
	cd sbin; make

extra_driver:
	cd drivers; make

clean:
	cd libs; make clean
	cd bin; make clean
	cd sbin; make clean
	cd drivers; make clean
