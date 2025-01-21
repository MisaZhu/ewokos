
export ARCH=arm
export HW=raspix

all: 
	cd libs/xpt2046; make
	cd libs/ili9486; make
	cd drivers/epaperd; make
	cd drivers/rpi_lcdd; make
	cd drivers/xpt2046d; make
	cd drivers/ili9486d; make
	cd drivers/hat13_joykeybd; make
	cd drivers/hat13_gamekbd; make
	cd drivers/lcdhatd; make
	cd drivers/gt911_touchd; make

clean:	
	cd libs/xpt2046; make clean
	cd libs/ili9486; make clean
	cd drivers/rpi_lcdd; make clean
	cd drivers/xpt2046d; make clean
	cd drivers/ili9486d; make clean
	cd drivers/epaperd; make clean
	cd drivers/hat13_joykeybd; make clean
	cd drivers/hat13_gamekbd; make clean
	cd drivers/lcdhatd; make clean
	cd drivers/gt911_touchd; make clean
