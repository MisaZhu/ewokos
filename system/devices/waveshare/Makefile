BUILD_DIR = ../../build
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = .

all: 
	cd drivers/epaperd; make
	cd drivers/hdmi_xpt2046d; make
	cd drivers/joystickd; make
	cd drivers/lcdhatd; make

clean:	
	cd drivers/epaperd; make clean
	cd drivers/hdmi_xpt2046d; make clean
	cd drivers/joystickd; make clean
	cd drivers/lcdhatd; make clean
