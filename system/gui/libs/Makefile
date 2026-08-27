all: basic_libs
	@echo "all done."

DIRS = minimp3 keyb mouse libpng libjpeg libgif libtga libsvg freetype g2dclient graph font display displayman displayd textgrid gterminal libogg libvorbis libiconbuf
basic_libs: $(DIRS)

# header dependencies between libs
graph: g2dclient libpng libjpeg libgif libtga libsvg
font: freetype
font display libiconbuf: graph
displayman displayd: display
textgrid gterminal: font graph
gterminal: textgrid

$(DIRS):
	@$(MAKE) -C $@

.PHONY: all basic_libs clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir clean; \
	done
