all:
	mkdir -p ../../build/rootfs/drivers/miyoo
	cd drivers/ms_uartd; make
	cd drivers/fbd; make
	cd drivers/gpio_joykeybd; make

clean:
	cd drivers/ms_uartd; make clean
	cd drivers/fbd; make clean
	cd drivers/gpio_joykeybd; make clean
