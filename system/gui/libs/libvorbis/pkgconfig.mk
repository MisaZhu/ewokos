
CFLAGS += -I../libvorbis/include -I../libogg/include

LDFLAGS += -L../libvorbis -L../libogg

LIBS := -lvorbis -logg $(LIBS)
LIBS_DEP += ../libvorbis/libvorbis.a ../libogg/libogg.a
