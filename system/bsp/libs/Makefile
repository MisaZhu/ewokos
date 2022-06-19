all: basic_libs
	@echo "all done."

basic_libs:
	cd arch_bcm283x;make
	cd arch_miyoo;make
	cd bsp;make

clean:
	cd arch_bcm283x;make clean
	cd arch_miyoo;make clean
	cd bsp;make clean