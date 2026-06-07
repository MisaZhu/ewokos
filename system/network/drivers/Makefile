all: basic_drivers
	@echo "all done."

basic_drivers:
	cd netd;$(MAKE)
	cd timed;$(MAKE)

clean:
	cd netd;$(MAKE) clean
	cd timed;$(MAKE) clean
