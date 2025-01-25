
export ARCH=arm
export HW=raspix
export MF=waveshare

ROOT_DIR = ../../../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

all: 
	mkdir -p $(BUILD_DIR)/$(HW)/drivers/$(MF)
	cd drivers/ili9486d; make

clean:	
	cd drivers/ili9486d; make clean
