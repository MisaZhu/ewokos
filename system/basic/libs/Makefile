all: basic_libs
	@echo "all done."

basic_libs:
	cd softfloat;make
	cd libc;make
	cd sconf; make
	cd ext2;make
	cd elf;make
	cd sd;make
	cd socket;make

clean:
	cd softfloat;make clean
	cd libc;make clean
	cd sconf; make clean
	cd ext2;make clean
	cd elf;make clean
	cd sd;make clean
	cd socket;make clean
