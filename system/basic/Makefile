BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..

all: mkdirs basic_libs builtin_apps basic_drivers
	@echo "all done."

basic_libs:
	cd libs;make

basic_drivers:
	cd drivers;make

mkdirs:
	@mkdir -p ${TARGET_DIR}/sbin
	@mkdir -p ${TARGET_DIR}/etc
	@mkdir -p ${TARGET_DIR}/bin
	@mkdir -p ${TARGET_DIR}/drivers/proc
	@mkdir -p ${TARGET_DIR}/home/root
	@mkdir -p ${TARGET_DIR}/home/guest

builtin_apps: 
	cd sbin;make
	cd bin;make

clean:
	cd sbin;make clean
	cd bin;make clean
	cd libs;make clean
	cd drivers;make clean
