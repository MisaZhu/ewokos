
CFLAGS += -I../SDL2/include

LDFLAGS += -L../SDL2

LIBS := -lSDL2 $(LIBS)
LIBS_DEP += ../SDL2/libSDL2.a
