all:
	cd versatilepb; make
	cd raspix; make
	cd miyoo; make
	cd rk3326; make
	cd rk3128; make
ifeq (${ARCH},riscv)
	cd virt; make
	cd nezha; make
endif


clean:
	cd versatilepb; make clean
	cd raspix; make clean
	cd miyoo; make clean
	cd rk3326; make clean
	cd rk3128; make clean
ifeq (${ARCH},riscv)
	cd virt; make
	cd nezha; make clean
endif
