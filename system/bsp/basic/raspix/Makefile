all: 
	mkdir -p ../../../build/rootfs/drivers/raspix
	cd drivers/mini_uartd; make
	cd drivers/pl011_uartd; make

clean:	
	cd drivers/mini_uartd; make clean
	cd drivers/pl011_uartd; make clean
