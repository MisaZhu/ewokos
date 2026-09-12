all: basic_libs
	@echo "all done."

DIRS = sd ext2 ext3 fat32 elf tinyjson openlibm zlib gpio hid usb c++/c++ c++/stl c++/object++
basic_libs: $(DIRS)

# header dependencies between libs
ext2 ext3: sd
usb: hid
c++/object++: tinyjson
c++/stl c++/object++: c++/c++

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all basic_libs clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
