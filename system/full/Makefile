BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

all: mkdirs extra_libs extra_driver extra_bin x_sbin x_bin
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/x
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/bin/x
	@cp -r ${ROOT_DIR}/etc ${TARGET_DIR}/$(ROOT_DIR)
	@cp -r ${ROOT_DIR}/data ${TARGET_DIR}/$(ROOT_DIR)

EXTRA_LIBS_DIR=libs
EXTRA_LIBXX_DIR=libs/cxx
extra_libs:
	cd $(EXTRA_LIBS_DIR)/ext2; make
	cd $(EXTRA_LIBS_DIR)/sd; make
	cd $(EXTRA_LIBS_DIR)/graph; make
	cd $(EXTRA_LIBS_DIR)/fonts; make
	cd $(EXTRA_LIBS_DIR)/fb; make
	cd $(EXTRA_LIBS_DIR)/console; make
	cd $(EXTRA_LIBS_DIR)/upng; make
	cd $(EXTRA_LIBS_DIR)/sconf; make
	cd $(EXTRA_LIBS_DIR)/tiny-json; make
	cd $(EXTRA_LIBS_DIR)/x; make
	cd $(EXTRA_LIBXX_DIR)/cxx; make
	cd $(EXTRA_LIBXX_DIR)/graphxx; make
	cd $(EXTRA_LIBXX_DIR)/x++; make

EXTRA_BIN_DIR=bin
extra_bin:
	cd $(EXTRA_BIN_DIR)/dump;make
	cd $(EXTRA_BIN_DIR)/global;make
	cd $(EXTRA_BIN_DIR)/grep;make
	cd $(EXTRA_BIN_DIR)/kill;make
	cd $(EXTRA_BIN_DIR)/mkdir;make
	cd $(EXTRA_BIN_DIR)/rm;make
	cd $(EXTRA_BIN_DIR)/fbtool;make

X_DIR=x
x_bin:
	cd $(X_DIR)/bin/gtest;make
	cd $(X_DIR)/bin/snake;make
	cd $(X_DIR)/bin/launcher;make
	cd $(X_DIR)/bin/xconsole;make
	cd $(X_DIR)/bin/png;make

x_sbin:
	cd $(X_DIR)/sbin/xmouse;make
	cd $(X_DIR)/sbin/xtouch;make
	cd $(X_DIR)/sbin/xim/vkey;make
	cd $(X_DIR)/sbin/xim/none;make
	cd $(X_DIR)/sbin/xjoystick;make
	cd $(X_DIR)/sbin/xwm/macos7;make
	cd $(X_DIR)/sbin/xwm/win3;make

EXTRA_DRIVERS_DIR=drivers
extra_driver:
	cd $(EXTRA_DRIVERS_DIR)/consoled;make
	cd $(EXTRA_DRIVERS_DIR)/sdfsd;make
	cd $(EXTRA_DRIVERS_DIR)/xserverd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/versatilepb/fbd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/versatilepb/ps2keybd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/versatilepb/ps2moused;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/fbd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/gpio_actledd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/mbox_actledd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/joystickd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/lcdhatd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/epaperd;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/hdmi_xpt2046d;make
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/usbd;make

clean:
	cd $(EXTRA_LIBS_DIR)/sd; make clean
	cd $(EXTRA_LIBS_DIR)/ext2; make clean
	cd $(EXTRA_LIBS_DIR)/graph; make clean
	cd $(EXTRA_LIBS_DIR)/fb; make clean
	cd $(EXTRA_LIBS_DIR)/console; make clean
	cd $(EXTRA_LIBS_DIR)/fonts; make clean
	cd $(EXTRA_LIBS_DIR)/x; make clean
	cd $(EXTRA_LIBS_DIR)/upng; make clean
	cd $(EXTRA_LIBS_DIR)/sconf; make clean
	cd $(EXTRA_LIBS_DIR)/tiny-json; make clean
	cd $(EXTRA_LIBXX_DIR)/cxx; make clean
	cd $(EXTRA_LIBXX_DIR)/graphxx; make clean
	cd $(EXTRA_LIBXX_DIR)/x++; make clean
	cd $(EXTRA_DRIVERS_DIR)/consoled;make clean
	cd $(EXTRA_DRIVERS_DIR)/sdfsd;make clean
	cd $(EXTRA_DRIVERS_DIR)/xserverd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/versatilepb/fbd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/versatilepb/ps2keybd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/versatilepb/ps2moused;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/fbd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/gpio_actledd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/mbox_actledd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/joystickd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/lcdhatd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/epaperd;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/hdmi_xpt2046d;make clean
	cd $(EXTRA_DRIVERS_DIR)/arch/arm/raspix/usbd;make clean
	cd $(X_DIR)/sbin/xmouse;make clean
	cd $(X_DIR)/sbin/xtouch;make clean
	cd $(X_DIR)/sbin/xim/vkey;make clean
	cd $(X_DIR)/sbin/xim/none;make clean
	cd $(X_DIR)/sbin/xjoystick;make clean
	cd $(X_DIR)/sbin/xwm/macos7;make clean
	cd $(X_DIR)/sbin/xwm/win3;make clean
	cd $(X_DIR)/bin/gtest;make clean
	cd $(X_DIR)/bin/snake;make clean
	cd $(X_DIR)/bin/launcher;make clean
	cd $(X_DIR)/bin/xconsole;make clean
	cd $(X_DIR)/bin/png;make clean
	cd $(EXTRA_BIN_DIR)/dump;make clean
	cd $(EXTRA_BIN_DIR)/global;make clean
	cd $(EXTRA_BIN_DIR)/grep;make clean
	cd $(EXTRA_BIN_DIR)/kill;make clean
	cd $(EXTRA_BIN_DIR)/mkdir;make clean
	cd $(EXTRA_BIN_DIR)/rm;make clean
	cd $(EXTRA_BIN_DIR)/fbtool;make clean
