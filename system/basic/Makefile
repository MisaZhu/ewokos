BUILD_DIR = ../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..

all: mkdirs basic_libs builtin_apps basic_drivers
	@echo "all done."

basic_libs:
	cd libc;make
	cd libs;make

basic_drivers:
	cd drivers;make

mkdirs:
	@mkdir -p ${TARGET_DIR}/sbin
	@mkdir -p ${TARGET_DIR}/etc
	@mkdir -p ${TARGET_DIR}/bin
	@mkdir -p ${TARGET_DIR}/drivers/proc
	@mkdir -p ${TARGET_DIR}/home/root
	@chmod 750 ${TARGET_DIR}/home/root
	@cp -r etc ${TARGET_DIR}

builtin_apps: 
	cd sys;make
	cd sbin;make
	cd bin;make

clean:
	cd sys;make clean
	cd sbin;make clean
	cd bin;make clean
	cd libc;make clean
	cd libs;make clean
	cd drivers;make clean
