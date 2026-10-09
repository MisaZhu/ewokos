
CFLAGS += -I../SDL2_mixer

LDFLAGS += -L../SDL2_mixer

LIBS := -lSDL2_mixer $(LIBS)
LIBS_DEP += ../SDL2_mixer/libSDL2_mixer.a
