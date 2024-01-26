all: basic_libs
	@echo "all done."

basic_libs:
	cd softfloat;make
ifeq ($(LIBEWOKC),yes)
	cd libewokc;make
else
	cd libewoksys;make
	cd libgloss;make
endif
	cd sconf; make
	cd ext2;make
	cd elf;make
	cd sd;make
	cd socket;make

clean:
	cd softfloat;make clean
ifeq ($(LIBEWOKC),yes)
	cd libewokc;make clean
else
	cd libewoksys;make clean
	cd libgloss;make clean
endif
	cd sconf; make clean
	cd ext2;make clean
	cd elf;make clean
	cd sd;make clean
	cd socket;make clean
