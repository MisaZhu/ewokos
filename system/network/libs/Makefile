all: basic_libs
	@echo "all done."

basic_libs:
	cd socket;$(MAKE)
	cd ntpc;$(MAKE)
	cd libtinyhttpsc;$(MAKE)

clean:
	cd socket;$(MAKE) clean
	cd ntpc;$(MAKE) clean
	cd libtinyhttpsc;$(MAKE) clean
