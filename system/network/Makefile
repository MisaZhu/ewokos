BUILD_DIR = ../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..


all: mkdirs network_libs bins sbins network_driver 
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/drivers

network_libs:
	cd libs; make

bins:
	cd bin; make

sbins:
	cd sbin; make

network_driver:
	cd drivers; make

clean:
	cd libs; make clean
	cd bin; make clean
	cd sbin; make clean
	cd drivers; make clean
