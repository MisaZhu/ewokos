all: basic_libs
	@echo "all done."

basic_libs:
ifneq ($(ARCH),aarch64)
	cd softfloat;$(MAKE)
endif
	cd libewoksys;$(MAKE)
	cd libgloss;$(MAKE)

clean:
	cd softfloat;$(MAKE) clean
	cd libewoksys;$(MAKE) clean
	cd libgloss;$(MAKE) clean