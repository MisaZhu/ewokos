BUILD_DIR = ../build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..


all: mkdirs network_libs bins sbins network_driver 
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/drivers ${TARGET_DIR}/etc
	@cp -Rp etc/. ${TARGET_DIR}/etc/

network_libs:
	cd libs; $(MAKE)

bins: mkdirs network_libs
	cd bin; $(MAKE)

sbins: mkdirs network_libs
	cd sbin; $(MAKE)

network_driver: mkdirs network_libs
	cd drivers; $(MAKE)

CLEAN_DIRS = libs bin sbin drivers
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
