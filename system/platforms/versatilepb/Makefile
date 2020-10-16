all: 
	mkdir -p ../../build/rootfs/drivers/versatilepb
	cd libs/vpb; make
	cd drivers/ttyd; make
	cd drivers/fbd; make
	cd drivers/ps2keybd; make
	cd drivers/ps2moused; make

clean:	
	cd libs/vpb; make clean
	cd drivers/ttyd; make clean
	cd drivers/fbd; make clean
	cd drivers/ps2keybd; make clean
	cd drivers/ps2moused; make clean
