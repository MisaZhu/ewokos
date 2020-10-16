BUILD_DIR = ../../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

all: 
	cd bin/gtest; make
	cd bin/snake; make
	cd bin/png; make
	cd sbin/xjoystick; make
	cd sbin/xtouch; make
	cd sbin/xim/vkey; make
	cd sbin/xwm/win3; make

clean:	
	cd bin/gtest; make clean
	cd bin/snake; make clean
	cd bin/png; make clean
	cd sbin/xjoystick; make clean
	cd sbin/xtouch; make clean
	cd sbin/xim/vkey; make clean
	cd sbin/xwm/win3; make clean
