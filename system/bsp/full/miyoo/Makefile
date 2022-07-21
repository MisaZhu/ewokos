all:
	mkdir -p ../../../build/rootfs/drivers/miyoo
	cd drivers/fbd; make
	cd drivers/gpio_joykeybd; make
	cd drivers/audio; make

clean:
	cd drivers/fbd; make clean
	cd drivers/gpio_joykeybd; make clean
	cd drivers/audio; make clean
