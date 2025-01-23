
export ARCH=arm
export HW=raspix
export MF=hat1.3

ROOT_DIR = ../../../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

all: 
	mkdir -p $(BUILD_DIR)/$(HW)/drivers/$(MF)
	cd drivers/hat13_joykeybd; make
	cd drivers/hat13_gamekbd; make
	cd drivers/lcdhatd; make
	cp -r etc $(TARGET_DIR)/

clean:	
	cd drivers/hat13_joykeybd; make clean
	cd drivers/hat13_gamekbd; make clean
	cd drivers/lcdhatd; make clean
