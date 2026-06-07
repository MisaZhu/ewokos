all: basic_libs
	@echo "all done."

basic_libs:
	cd sd;$(MAKE)
	cd ext2;$(MAKE)
	cd elf;$(MAKE)
	cd tinyjson;$(MAKE)
	cd zlib;$(MAKE)
	cd gpio;$(MAKE)
	cd c++/stl;$(MAKE)
	cd c++/c++;$(MAKE)
	cd c++/object++;$(MAKE)

clean:
	cd ext2;$(MAKE) clean
	cd elf;$(MAKE) clean
	cd sd;$(MAKE) clean
	cd tinyjson;$(MAKE) clean
	cd gpio;$(MAKE) clean
	cd zlib;$(MAKE) clean
	cd c++/stl;$(MAKE) clean
	cd c++/c++;$(MAKE) clean
