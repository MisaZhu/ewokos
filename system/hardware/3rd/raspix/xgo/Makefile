
export ARCH=arm
export HW=raspix
export MF=xgo

ROOT_DIR = ../../../..
BUILD_DIR = $(ROOT_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

all: 
	mkdir -p $(BUILD_DIR)/$(HW)/drivers/$(MF)
	cd drivers/spilcdd; make
	cd drivers/buttond; make
	cp -r etc $(TARGET_DIR)/

clean:	
	cd drivers/spilcdd; make clean
	cd drivers/buttond; make clean
