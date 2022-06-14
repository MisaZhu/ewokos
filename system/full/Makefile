BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..

all: mkdirs extra_libs extra_driver extra_bin x_sbin x_bin
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/sbin/x
	@mkdir -p ${TARGET_DIR}/bin/x
	@cp -r ${ROOT_DIR}/etcs/qemu/full/* ${TARGET_DIR}/etc

EXTRA_LIBS_DIR=libs
EXTRA_LIBXX_DIR=libs/cxx
extra_libs:
	cd $(EXTRA_LIBS_DIR)/x; make
	cd $(EXTRA_LIBXX_DIR)/cxx; make
	cd $(EXTRA_LIBXX_DIR)/x++; make

EXTRA_BIN_DIR=bin
extra_bin:

X_DIR=x
x_bin:
	cd $(X_DIR)/bin/gtest;make
	cd $(X_DIR)/bin/ttyjoy;make
	cd $(X_DIR)/bin/png;make
	cd $(X_DIR)/bin/launcher;make
	cd $(X_DIR)/bin/xconsole;make
	cd $(X_DIR)/bin/finder;make

x_sbin:
	cd $(X_DIR)/sbin/xmouse;make
	cd $(X_DIR)/sbin/xjoystick;make
	cd $(X_DIR)/sbin/xtouch;make
	cd $(X_DIR)/sbin/xim/none;make
	cd $(X_DIR)/sbin/xim/vkey;make
	cd $(X_DIR)/sbin/xwm/macos7;make

EXTRA_DRIVERS_DIR=drivers
extra_driver:
	cd $(EXTRA_DRIVERS_DIR)/xserverd;make

clean:
	cd $(EXTRA_LIBS_DIR)/x; make clean
	cd $(EXTRA_LIBXX_DIR)/cxx; make clean
	cd $(EXTRA_LIBXX_DIR)/x++; make clean
	cd $(EXTRA_DRIVERS_DIR)/xserverd;make clean
	cd $(X_DIR)/sbin/xmouse;make clean
	cd $(X_DIR)/sbin/xjoystick;make clean
	cd $(X_DIR)/sbin/xtouch;make clean
	cd $(X_DIR)/sbin/xim/none;make clean
	cd $(X_DIR)/sbin/xim/vkey;make clean
	cd $(X_DIR)/sbin/xwm/macos7;make clean
	cd $(X_DIR)/bin/gtest;make clean
	cd $(X_DIR)/bin/png;make clean
	cd $(X_DIR)/bin/launcher;make clean
	cd $(X_DIR)/bin/xconsole;make clean
	cd $(X_DIR)/bin/ttyjoy;make clean
	cd $(X_DIR)/bin/finder;make clean
