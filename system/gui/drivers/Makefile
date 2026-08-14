all: drivers
	@echo "all done."

drivers:
	cd fontd;$(MAKE)
	cd displaymand;$(MAKE)
	cd consoled;$(MAKE)

clean:
	cd fontd;$(MAKE) clean
	cd displaymand;$(MAKE) clean
	cd consoled;$(MAKE) clean