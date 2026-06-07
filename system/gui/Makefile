BUILD_DIR = ../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..


all: mkdirs extra_libs bins sbins extra_driver 
	@echo "all done."

mkdirs:
	@mkdir -p ${TARGET_DIR}/drivers
	@cp -r usr ${TARGET_DIR}
	@cp -r etc ${TARGET_DIR}

extra_libs:
	cd libs; $(MAKE)

bins: extra_libs
	cd bin; $(MAKE)

sbins: extra_libs
	cd sbin; $(MAKE)

extra_driver: extra_libs
	cd drivers; $(MAKE)

clean:
	cd libs; $(MAKE) clean
	cd bin; $(MAKE) clean
	cd sbin; $(MAKE) clean
	cd drivers; $(MAKE) clean
