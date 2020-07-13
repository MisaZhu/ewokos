BUILD_DIR = ../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

all: mkdirs extra_libs extra_apps extra_sbin
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/dev/raspi
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/sbin/x
	@mkdir -p ${TARGET_DIR}/$(ROOT_DIR)/apps
	@cp -r ${ROOT_DIR}/etc ${TARGET_DIR}/$(ROOT_DIR)
	@cp -r ${ROOT_DIR}/data ${TARGET_DIR}/$(ROOT_DIR)

EXTRA_LIBS_DIR=libs
EXTRA_LIBXX_DIR=libxx
extra_libs:
	cd $(EXTRA_LIBS_DIR)/upng; make
	cd $(EXTRA_LIBS_DIR)/sconf; make
	cd $(EXTRA_LIBS_DIR)/tiny-json; make
	cd $(EXTRA_LIBS_DIR)/x; make
	cd $(EXTRA_LIBXX_DIR)/cxx; make
	cd $(EXTRA_LIBXX_DIR)/graphxx; make
	cd $(EXTRA_LIBXX_DIR)/x++; make

APPS_DIR=apps
extra_apps:
	cd $(APPS_DIR)/gtest;make
	cd $(APPS_DIR)/snake;make
	cd $(APPS_DIR)/launcher;make
	cd $(APPS_DIR)/syspanel;make
	cd $(APPS_DIR)/xconsole;make
	cd $(APPS_DIR)/png;make

EXTRA_SBIN_DIR=sbin
extra_sbin:
	cd $(EXTRA_SBIN_DIR)/x/xmouse;make
	cd $(EXTRA_SBIN_DIR)/x/xim;make
	cd $(EXTRA_SBIN_DIR)/x/xjoystick;make
	cd $(EXTRA_SBIN_DIR)/x/xwm/macos7;make
	cd $(EXTRA_SBIN_DIR)/x/xwm/win3;make
	cd $(EXTRA_SBIN_DIR)/dev/xserverd;make
	cd $(EXTRA_SBIN_DIR)/dev/arch/raspi/joystickd;make
	cd $(EXTRA_SBIN_DIR)/dev/arch/raspi/lcdhatd;make
	cd $(EXTRA_SBIN_DIR)/dev/arch/raspi/ttyd;make

clean:
	cd $(APPS_DIR)/gtest;make clean
	cd $(APPS_DIR)/snake;make clean
	cd $(APPS_DIR)/launcher;make clean
	cd $(APPS_DIR)/syspanel;make clean
	cd $(APPS_DIR)/xconsole;make clean
	cd $(APPS_DIR)/png;make clean
	cd $(EXTRA_LIBS_DIR)/x; make clean
	cd $(EXTRA_LIBS_DIR)/upng; make clean
	cd $(EXTRA_LIBS_DIR)/sconf; make clean
	cd $(EXTRA_LIBS_DIR)/tiny-json; make clean
	cd $(EXTRA_LIBXX_DIR)/cxx; make clean
	cd $(EXTRA_LIBXX_DIR)/graphxx; make clean
	cd $(EXTRA_LIBXX_DIR)/x++; make clean
	cd $(EXTRA_SBIN_DIR)/x/xmouse;make clean
	cd $(EXTRA_SBIN_DIR)/x/xim;make clean
	cd $(EXTRA_SBIN_DIR)/x/xjoystick;make clean
	cd $(EXTRA_SBIN_DIR)/x/xwm/macos7;make clean
	cd $(EXTRA_SBIN_DIR)/x/xwm/win3;make clean
	cd $(EXTRA_SBIN_DIR)/dev/xserverd;make clean
	cd $(EXTRA_SBIN_DIR)/dev/arch/raspi/joystickd;make clean
	cd $(EXTRA_SBIN_DIR)/dev/arch/raspi/lcdhatd;make clean
	cd $(EXTRA_SBIN_DIR)/dev/arch/raspi/ttyd;make clean
