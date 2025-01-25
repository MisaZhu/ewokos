
export ARCH=arm
export HW=raspix
export MF=waveshare

ROOT_DIR = ../../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

all: 
	mkdir -p $(BUILD_DIR)/$(HW)/drivers/$(MF)
	cd drivers/joykeybd; make
	cd drivers/gamekbd; make
	cd drivers/lcdhatd; make
	cp -r etc $(TARGET_DIR)/

clean:	
	cd drivers/joykeybd; make clean
	cd drivers/gamekbd; make clean
	cd drivers/lcdhatd; make clean
