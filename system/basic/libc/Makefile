all: basic_libs
	@echo "all done."

DIRS = libewoksys libgloss
basic_libs:
ifeq ($(ARCH),arm)
	cd softfloat;$(MAKE)
endif
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = softfloat libewoksys libgloss
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done