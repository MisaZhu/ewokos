export ARCH=arm
export HW=raspix

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/$(HW)

all: 
	cd drivers/beep; make
	cd drivers/leds; make
	cd drivers/keys; make
	cd drivers/disp; make

clean:	
	cd drivers/beep; make clean
	cd drivers/leds; make clean
	cd drivers/keys; make clean
	cd drivers/disp; make clean
