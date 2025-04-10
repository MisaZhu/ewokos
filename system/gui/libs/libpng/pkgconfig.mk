
CFLAGS += -I../libpng -I../zlib

LDFLAGS += -L../libpng -L../zlib

LIBS := -lpng -lz $(LIBS)
LIBS_DEP += ../libpng/libpng.a ../zlib/libz.a
