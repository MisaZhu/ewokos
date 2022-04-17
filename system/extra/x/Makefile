BUILD_DIR = ../../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

all: 
	cd bin/snake; make
	cd bin/png; make
	cd sbin/xwm/win3; make

clean:	
	cd bin/snake; make clean
	cd bin/png; make clean
	cd sbin/xwm/win3; make clean
