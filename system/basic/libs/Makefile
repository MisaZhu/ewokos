all: basic_libs
	@echo "all done."

DIRS = sd ext2 ext3 fat32 elf tinyjson openlibm zlib gpio c++/c++ c++/stl c++/object++ dl
basic_libs: $(DIRS)

# header dependencies between libs
ext2 ext3: sd
c++/object++: tinyjson
c++/stl c++/object++: c++/c++
# dl needs <elf/elf_dyn.h>, which elf's Makefile installs into $(BUILD_DIR)/include.
# It also nm's libewoksys/libc/libgloss to build the host ABI export table, and
# system/basic/Makefile already orders BASIC_LIBS_DIRS = libc libs, so those
# archives exist by the time this runs.
dl: elf

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all basic_libs clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
