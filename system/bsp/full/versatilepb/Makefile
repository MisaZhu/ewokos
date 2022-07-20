all: 
	mkdir -p ../../../build/rootfs/drivers/versatilepb
	cd drivers/fbd; make
	cd drivers/ps2keybd; make
	cd drivers/ps2moused; make

clean:	
	cd drivers/fbd; make clean
	cd drivers/ps2keybd; make clean
	cd drivers/ps2moused; make clean
