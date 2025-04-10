all: basic_libs
	@echo "all done."

basic_libs:
	cd ext2;make
	cd elf;make
	cd sd;make
	cd socket;make
	cd tinyjson;make
	cd zlib;make
	cd gpio;make

clean:
	cd ext2;make clean
	cd elf;make clean
	cd sd;make clean
	cd socket;make clean 
	cd tinyjson;make clean
	cd gpio;make clean
	cd zlib;make clean
