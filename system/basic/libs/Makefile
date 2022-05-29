all: basic_libs
	@echo "all done."

basic_libs:
	cd libc;make
	cd zlib;make
	cd sconf; make
	cd hash;make
	cd ext2;make
	cd arch_bcm283x;make
	cd arch_vpb;make
	cd arch_miyoo;make
	cd sd;make
	cd display;make
	cd graph;make
	cd fb;make
	cd fbd;make
	cd fonts;make
	cd upng; make
	cd console;make

clean:
	cd libc;make clean
	cd zlib;make clean
	cd sconf; make clean
	cd hash;make clean
	cd ext2;make clean
	cd arch_bcm283x;make clean
	cd arch_vpb;make clean
	cd arch_miyoo;make clean
	cd sd;make clean
	cd fb;make clean
	cd fbd;make clean
	cd graph;make clean
	cd fonts;make clean
	cd console;make clean
	cd display;make clean
	cd upng; make clean
