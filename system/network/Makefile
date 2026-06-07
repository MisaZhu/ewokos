BUILD_DIR = ../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..


all: mkdirs network_libs bins sbins network_driver 
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/drivers

network_libs:
	cd libs; $(MAKE)

bins: network_libs
	cd bin; $(MAKE)

sbins: network_libs
	cd sbin; $(MAKE)

network_driver: network_libs
	cd drivers; $(MAKE)

clean:
	cd libs; $(MAKE) clean
	cd bin; $(MAKE) clean
	cd sbin; $(MAKE) clean
	cd drivers; $(MAKE) clean
