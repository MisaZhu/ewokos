all: basic_libs
	@echo "all done."

basic_libs:
	cd sd;make
	cd ext2;make
	cd elf;make
	cd tinyjson;make
	cd zlib;make
	cd gpio;make
	cd c++/stl;make
	cd c++/c++;make
	cd c++/object++;make

clean:
	cd ext2;make clean
	cd elf;make clean
	cd sd;make clean
	cd tinyjson;make clean
	cd gpio;make clean
	cd zlib;make clean
	cd c++/stl;make clean
	cd c++/c++;make clean
