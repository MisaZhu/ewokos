BUILD_DIR = ../../build
TARGET_DIR = $(BUILD_DIR)/rootfs/etc/dev/extra

all: 
	mkdir -p $(TARGET_DIR)
	cp waveshare.dev $(TARGET_DIR)
	cd drivers/epaperd; make
	cd drivers/xpt2046d; make
	cd drivers/ili9486d; make
	cd drivers/joystickd; make
	cd drivers/lcdhatd; make

clean:	
	cd drivers/epaperd; make clean
	cd drivers/xpt2046d; make clean
	cd drivers/ili9486d; make clean
	cd drivers/joystickd; make clean
	cd drivers/lcdhatd; make clean
	rm -f $(TARGET_DIR)/waveshare.dev
