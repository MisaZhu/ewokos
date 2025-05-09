all: basic_libs
	@echo "all done."

basic_libs:
ifneq ($(ARCH),aarch64)
	cd softfloat;make
endif
	cd libewoksys;make
	cd libgloss;make

clean:
	cd softfloat;make clean
	cd libewoksys;make clean
	cd libgloss;make clean