BUILD_DIR = ../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..

all: mkdirs basic_libs builtin_apps basic_drivers
	@echo "all done."

basic_libs:
	cd libc;$(MAKE)
	cd libs;$(MAKE)

basic_drivers: basic_libs
	cd drivers;$(MAKE)

mkdirs:
	@mkdir -p ${TARGET_DIR}/sbin
	@mkdir -p ${TARGET_DIR}/etc
	@mkdir -p ${TARGET_DIR}/bin
	@mkdir -p ${TARGET_DIR}/drivers/proc
	@mkdir -p ${TARGET_DIR}/home/root
	@chmod 750 ${TARGET_DIR}/home/root
	@cp -r etc ${TARGET_DIR}

builtin_apps: basic_libs
	cd sys;$(MAKE)
	cd sbin;$(MAKE)
	cd bin;$(MAKE)

clean:
	cd sys;$(MAKE) clean
	cd sbin;$(MAKE) clean
	cd bin;$(MAKE) clean
	cd libc;$(MAKE) clean
	cd libs;$(MAKE) clean
	cd drivers;$(MAKE) clean
