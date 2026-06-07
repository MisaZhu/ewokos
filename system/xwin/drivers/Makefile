all: drivers
	@echo "all done."

drivers:
	cd xserverd;$(MAKE)
	cd vjoystickd;$(MAKE)
	cd vkeybd;$(MAKE)

clean:
	cd xserverd;$(MAKE) clean
	cd vjoystickd;$(MAKE) clean
	cd vkeybd;$(MAKE) clean