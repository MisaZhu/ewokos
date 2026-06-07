all: drivers
	@echo "all done."

drivers:
	cd fontd;$(MAKE)
	cd displayd;$(MAKE)
	cd consoled;$(MAKE)

clean:
	cd fontd;$(MAKE) clean
	cd displayd;$(MAKE) clean
	cd consoled;$(MAKE) clean