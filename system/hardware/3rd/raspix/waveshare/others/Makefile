
export ARCH=arm
export HW=raspix

all: 
	cd libs/xpt2046; make
	cd libs/ili9486; make
	cd drivers/epaperd; make
	cd drivers/rpi_lcdd; make
	cd drivers/xpt2046d; make
	cd drivers/ili9486d; make
	cd drivers/gt911_touchd; make

clean:	
	cd libs/xpt2046; make clean
	cd libs/ili9486; make clean
	cd drivers/rpi_lcdd; make clean
	cd drivers/xpt2046d; make clean
	cd drivers/ili9486d; make clean
	cd drivers/epaperd; make clean
	cd drivers/gt911_touchd; make clean
