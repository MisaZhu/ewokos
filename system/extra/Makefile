BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

all: mkdirs extra_libs extra_driver x_sbin x_bin
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/x
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/apps
	@cp -r ${ROOT_DIR}/etc ${TARGET_DIR}/$(ROOT_DIR)
	@cp -r ${ROOT_DIR}/data ${TARGET_DIR}/$(ROOT_DIR)

EXTRA_LIBS_DIR=libs
EXTRA_LIBXX_DIR=libs/cxx
extra_libs:
	cd $(EXTRA_LIBS_DIR)/fonts; make
	cd $(EXTRA_LIBS_DIR)/upng; make
	cd $(EXTRA_LIBS_DIR)/sconf; make
	cd $(EXTRA_LIBS_DIR)/tiny-json; make
	cd $(EXTRA_LIBS_DIR)/x; make
	cd $(EXTRA_LIBXX_DIR)/cxx; make
	cd $(EXTRA_LIBXX_DIR)/graphxx; make
	cd $(EXTRA_LIBXX_DIR)/x++; make

X_DIR=x
x_bin:
	cd $(X_DIR)/bin/gtest;make
	cd $(X_DIR)/bin/snake;make
	cd $(X_DIR)/bin/launcher;make
	cd $(X_DIR)/bin/syspanel;make
	cd $(X_DIR)/bin/xconsole;make
	cd $(X_DIR)/bin/png;make

x_sbin:
	cd $(X_DIR)/sbin/xmouse;make
	cd $(X_DIR)/sbin/xim/vkey;make
	cd $(X_DIR)/sbin/xim/none;make
	cd $(X_DIR)/sbin/xjoystick;make
	cd $(X_DIR)/sbin/xwm/macos7;make
	cd $(X_DIR)/sbin/xwm/win3;make

EXTRA_DRIVERS_DIR=drivers
extra_driver:
	cd $(EXTRA_DRIVERS_DIR)/xserverd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/raspi/joystickd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/raspi/lcdhatd;make

clean:
	cd $(EXTRA_LIBS_DIR)/fonts; make clean
	cd $(EXTRA_LIBS_DIR)/x; make clean
	cd $(EXTRA_LIBS_DIR)/upng; make clean
	cd $(EXTRA_LIBS_DIR)/sconf; make clean
	cd $(EXTRA_LIBS_DIR)/tiny-json; make clean
	cd $(EXTRA_LIBXX_DIR)/cxx; make clean
	cd $(EXTRA_LIBXX_DIR)/graphxx; make clean
	cd $(EXTRA_LIBXX_DIR)/x++; make clean
	cd $(EXTRA_DRIVERS_DIR)/xserverd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/raspi/joystickd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/raspi/lcdhatd;make clean
	cd $(X_DIR)/sbin/xmouse;make clean
	cd $(X_DIR)/sbin/xim/vkey;make clean
	cd $(X_DIR)/sbin/xim/none;make clean
	cd $(X_DIR)/sbin/xjoystick;make clean
	cd $(X_DIR)/sbin/xwm/macos7;make clean
	cd $(X_DIR)/sbin/xwm/win3;make clean
	cd $(X_DIR)/bin/gtest;make clean
	cd $(X_DIR)/bin/snake;make clean
	cd $(X_DIR)/bin/launcher;make clean
	cd $(X_DIR)/bin/syspanel;make clean
	cd $(X_DIR)/bin/xconsole;make clean
	cd $(X_DIR)/bin/png;make clean
