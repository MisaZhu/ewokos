all: x_libs
	@echo "all done."

x_libs:
	cd x;$(MAKE)
	cd x++;$(MAKE)
	cd widget++;$(MAKE)

clean:
	cd x;$(MAKE) clean
	cd x++;$(MAKE) clean
	cd widget++;$(MAKE) clean