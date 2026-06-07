all: basic_libs
	@echo "all done."

basic_libs:
	cd arch_virt;$(MAKE)
	cd bsp;$(MAKE)

clean:
	cd arch_virt;$(MAKE) clean
	cd bsp;$(MAKE) clean
