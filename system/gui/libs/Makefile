all: basic_libs
	@echo "all done."

basic_libs:
	cd openlibm;$(MAKE)
	cd minimp3;$(MAKE)
	cd keyb;$(MAKE)
	cd mouse;$(MAKE)
	cd libpng; $(MAKE)
	cd libjpeg; $(MAKE)
	cd libgif; $(MAKE)
	cd libtga; $(MAKE)
	cd libsvg; $(MAKE)
	cd freetype; $(MAKE)
	cd graph;$(MAKE)
	cd font;$(MAKE)
	cd fb;$(MAKE)
	cd display;$(MAKE)
	cd fbd;$(MAKE)
	cd textgrid;$(MAKE)
	cd gterminal;$(MAKE)
	cd libogg;$(MAKE)
	cd libvorbis;$(MAKE)
	cd libiconbuf;$(MAKE)

clean:
	cd openlibm;$(MAKE) clean
	cd gterminal;$(MAKE) clean
	cd textgrid;$(MAKE) clean
	cd display;$(MAKE) clean
	cd libpng; $(MAKE) clean
	cd libjpeg; $(MAKE) clean
	cd libgif; $(MAKE) clean
	cd libtga; $(MAKE) clean
	cd libsvg; $(MAKE) clean
	cd freetype; $(MAKE) clean
	cd font;$(MAKE) clean
	cd minimp3;$(MAKE) clean
	cd graph;$(MAKE) clean
	cd fb;$(MAKE) clean
	cd keyb;$(MAKE) clean
	cd mouse;$(MAKE) clean
	cd fbd;$(MAKE) clean
	cd libogg;$(MAKE) clean
	cd libvorbis;$(MAKE) clean
	cd libiconbuf;$(MAKE) clean
