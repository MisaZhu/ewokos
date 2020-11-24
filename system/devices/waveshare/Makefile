BUILD_DIR = ../../build
TARGET_DIR = $(BUILD_DIR)/rootfs/etc/dev/extra

all: 
	mkdir -p $(TARGET_DIR)
	cp waveshare.dev $(TARGET_DIR)
	cd libs/xpt2046; make
	cd libs/ili9486; make
	cd drivers/epaperd; make
	cd drivers/rpi_lcdd; make
	cd drivers/xpt2046d; make
	cd drivers/ili9486d; make
	cd drivers/hat13_joystickd; make
	cd drivers/hat13_joykeybd; make
	cd drivers/gpio_joystickd; make
	cd drivers/gpio_joykeybd; make
	cd drivers/lcdhatd; make

clean:	
	cd libs/xpt2046; make clean
	cd libs/ili9486; make clean
	cd drivers/rpi_lcdd; make clean
	cd drivers/xpt2046d; make clean
	cd drivers/ili9486d; make clean
	cd drivers/epaperd; make clean
	cd drivers/hat13_joystickd; make clean
	cd drivers/hat13_joykeybd; make clean
	cd drivers/gpio_joystickd; make clean
	cd drivers/gpio_joykeybd; make clean
	cd drivers/lcdhatd; make clean
	rm -f $(TARGET_DIR)/waveshare.dev
