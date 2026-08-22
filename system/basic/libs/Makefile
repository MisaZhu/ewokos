all: basic_libs
	@echo "all done."

DIRS = sd ext2 ext3 fat32 elf tinyjson openlibm zlib gpio c++/c++ c++/stl c++/object++
basic_libs:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = ext2 ext3 fat32 elf sd tinyjson gpio openlibm zlib c++/stl c++/c++
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
