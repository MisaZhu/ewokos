BUILD_DIR = ../build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..


all: mkdirs extra_libs bins sbins extra_driver 
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/drivers ${TARGET_DIR}/usr ${TARGET_DIR}/etc
	@cp -Rp usr/. ${TARGET_DIR}/usr/
	@cp -Rp etc/. ${TARGET_DIR}/etc/

extra_libs:
	cd libs; $(MAKE)

bins: mkdirs extra_libs
	cd bin; $(MAKE)

sbins: mkdirs extra_libs
	cd sbin; $(MAKE)

extra_driver: mkdirs extra_libs
	cd drivers; $(MAKE)

CLEAN_DIRS = libs bin sbin drivers
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
