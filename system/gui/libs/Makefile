all: basic_libs
	@echo "all done."

DIRS = minimp3 keyb mouse libpng libjpeg libgif libtga libsvg freetype graph font display g2d displayman displayd textgrid gterminal libogg libvorbis libiconbuf
basic_libs:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir || exit 1; \
	done

CLEAN_DIRS = gterminal textgrid displayman libpng libjpeg libgif libtga libsvg freetype font minimp3 graph display g2d keyb mouse displayd libogg libvorbis libiconbuf
clean:
	@for dir in $(CLEAN_DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
