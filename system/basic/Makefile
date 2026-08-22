BUILD_DIR = ../build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs
ROOT_DIR = ..

all: mkdirs basic_libs builtin_apps basic_drivers
	@echo "all done."

BASIC_LIBS_DIRS = libc libs
basic_libs:
	@for dir in $(BASIC_LIBS_DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

basic_drivers: basic_libs
	cd drivers;$(MAKE)

mkdirs:
	@mkdir -p ${TARGET_DIR}/sbin
	@mkdir -p ${TARGET_DIR}/etc
	@mkdir -p ${TARGET_DIR}/bin
	@mkdir -p ${TARGET_DIR}/drivers/proc
	@mkdir -p ${TARGET_DIR}/home/root
	@chmod 750 ${TARGET_DIR}/home/root
	@cp -r etc ${TARGET_DIR}

BUILTIN_APPS_DIRS = sys sbin bin
builtin_apps: basic_libs
	@for dir in $(BUILTIN_APPS_DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = sys sbin bin libc libs drivers
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
