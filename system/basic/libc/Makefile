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

clean:
	cd softfloat;make clean
ifeq ($(LIBEWOKC),yes)
	cd libewokc;make clean
else
	cd libewoksys;make clean
	cd libgloss;make clean
endif