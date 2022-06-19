all: basic_libs
	@echo "all done."

basic_libs:
	cd softfloat;make
	cd libc;make
	cd zlib;make
	cd sconf; make
	cd hash;make
	cd crc32;make
	cd ext2;make
	cd sd;make

clean:
	cd softfloat;make clean
	cd libc;make clean
	cd zlib;make clean
	cd sconf; make clean
	cd hash;make clean
	cd crc32;make clean
	cd ext2;make clean
	cd sd;make clean