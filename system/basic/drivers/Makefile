all: basic_drivers
	@echo "all done."

basic_drivers:
	cd nulld;$(MAKE)
	cd logd;$(MAKE)
	cd ramfsd;$(MAKE)
	cd timerd;$(MAKE)

clean:
	cd nulld;$(MAKE) clean
	cd logd;$(MAKE) clean
	cd ramfsd;$(MAKE) clean
	cd timerd;$(MAKE) clean
