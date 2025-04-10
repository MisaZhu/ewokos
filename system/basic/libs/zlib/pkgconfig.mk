
CFLAGS += -I../zlib

LDFLAGS += -L../zlib

LIBS := -lz $(LIBS)
LIBS_DEP += ../zlib/libz.a
