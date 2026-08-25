all: basic_libs
	@echo "all done."

DIRS = minimp3 keyb mouse libpng libjpeg libgif libtga libsvg freetype graph font display g2dclient displayman displayd textgrid gterminal libogg libvorbis libiconbuf
basic_libs:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
