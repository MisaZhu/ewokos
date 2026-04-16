EWOKOS_ROOT_DIR = ../../..
include ../make.inc

SYS_BUILD_DIR=$(SYS_ROOT_DIR)/build/$(HW)
SDL2_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_IMG_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_MIXER_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_GFX_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_TTF_HEADS_DIR=$(SYS_BUILD_DIR)/include/SDL2
SDL2_LIB_DIR=$(SYS_BUILD_DIR)/lib

all:
	make -C SDL2
	make -C SDL2_image
	make -C SDL2_mixer
	make -C SDL2_gfx
	make -C SDL2_ttf
	mkdir -p $(SDL2_HEADS_DIR)
	mkdir -p $(SDL2_IMG_HEADS_DIR)
	mkdir -p $(SDL2_MIXER_HEADS_DIR)
	mkdir -p $(SDL2_GFX_HEADS_DIR)
	mkdir -p $(SDL2_TTF_HEADS_DIR)
	mkdir -p $(SDL2_LIB_DIR)
	cp SDL2/include/*.h $(SDL2_HEADS_DIR)
	cp SDL2_image/*.h $(SDL2_IMG_HEADS_DIR)
	cp SDL2_mixer/*.h $(SDL2_MIXER_HEADS_DIR)
	cp SDL2_gfx/*.h $(SDL2_GFX_HEADS_DIR)
	cp SDL2_ttf/*.h $(SDL2_GFX_HEADS_DIR)
	cp SDL2/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_image/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_mixer/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_gfx/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_ttf/lib*.a $(SDL2_LIB_DIR)

clean:
	make -C SDL2 -k clean
	make -C SDL2_image -k clean
	make -C SDL2_mixer -k clean
	make -C SDL2_gfx -k clean
	make -C SDL2_ttf -k clean
