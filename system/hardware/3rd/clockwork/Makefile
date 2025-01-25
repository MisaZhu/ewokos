
export ARCH=arm
export HW=raspix
export MF=clockwork

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/$(HW)

all: 
	mkdir -p $(BUILD_DIR)/$(HW)/drivers/$(MF)
	cd drivers/fbd; make
	cd drivers/usbd; make
	cd drivers/powerd; make
	cp -r etc/* $(TARGET_DIR)/etc

clean:	
	cd drivers/fbd; make clean
	cd drivers/usbd; make clean
	cd drivers/powerd; make clean