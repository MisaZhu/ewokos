all: basic_libs
	@echo "all done."

basic_libs:
	cd arch_bcm283x;make
	cd arch_miyoo;make
	cd arch_rk3326;make
	cd bsp;make

clean:
	cd arch_bcm283x;make clean
	cd arch_miyoo;make clean
	cd arch_rk3326;make clean
	cd bsp;make clean