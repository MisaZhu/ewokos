BUILD_DIR = ../../build
TARGET_DIR = $(BUILD_DIR)/rootfs/etc/dev/extra

all: 
	mkdir -p $(TARGET_DIR)
	cp nxez.dev $(TARGET_DIR)
	cd drivers/beep; make
	cd drivers/leds; make
	cd drivers/keys; make
	cd drivers/disp; make

clean:	
	cd drivers/beep; make clean
	cd drivers/leds; make clean
	cd drivers/keys; make clean
	cd drivers/disp; make clean
	rm -f $(TARGET_DIR)/nxez.dev
