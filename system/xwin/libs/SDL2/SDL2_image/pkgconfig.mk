
CFLAGS += -I../SDL2_image

LDFLAGS += -L../SDL2_image

LIBS := -lSDL2_image $(LIBS)
LIBS_DEP += ../SDL2_image/libSDL2_image.a
