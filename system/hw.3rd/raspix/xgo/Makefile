
export ARCH=arm
export HW=raspix
export MF=xgo

SYS_DIR = ../../..
BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

all: 
	mkdir -p $(BUILD_DIR)/$(HW)/drivers/$(MF)
	cd drivers/spilcdd; make
	cd drivers/buttond; make
	cd libs; make
	cd apps/xgo; make
	cp -r etc $(TARGET_DIR)/

clean:	
	cd drivers/spilcdd; make clean
	cd drivers/buttond; make clean
	cd libs; make clean
	cd apps/xgo; make clean
