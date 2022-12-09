all: basic_libs
	@echo "all done."

basic_libs:
	cd arch_vpb;make
	cd arch_bcm283x;make
	cd arch_miyoo;make
	cd arch_rk3326;make
	cd arch_rk3128;make
	cd arch_virt;make
	cd arch_nezha;make
	cd bsp;make

clean:
	cd arch_vpb;make clean
	cd arch_bcm283x;make clean
	cd arch_miyoo;make clean
	cd arch_rk3326;make clean
	cd arch_rk3128;make clean
	cd arch_virt;make clean
	cd arch_nezha;make clean
	cd bsp;make clean
