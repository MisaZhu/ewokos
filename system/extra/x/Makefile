BUILD_DIR = ../../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

all: 
	cd bin/snake; make
	cd bin/emu; make
	cd bin/gpiomonitor; make
	cd bin/anim; make

clean:	
	cd bin/snake; make clean
	cd bin/emu; make clean
	cd bin/gpiomonitor; make clean
	cd bin/anim; make clean
