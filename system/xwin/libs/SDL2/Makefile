SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

SDL2_HEADS_DIR=$(TARGET_DIR)/include/SDL2
SDL2_LIB_DIR=$(TARGET_DIR)/lib

# build chain: SDL2 core first, the add-on libs only need its headers
DIRS = SDL2 SDL2_image SDL2_mixer SDL2_gfx SDL2_ttf

all: $(DIRS)
	mkdir -p $(SDL2_HEADS_DIR)
	mkdir -p $(SDL2_LIB_DIR)
	cp SDL2/include/*.h $(SDL2_HEADS_DIR)
	cp SDL2_image/*.h $(SDL2_HEADS_DIR)
	cp SDL2_mixer/*.h $(SDL2_HEADS_DIR)
	cp SDL2_gfx/*.h $(SDL2_HEADS_DIR)
	cp SDL2_ttf/*.h $(SDL2_HEADS_DIR)
	cp SDL2/$(ARCH)/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_image/$(ARCH)/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_mixer/$(ARCH)/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_gfx/$(ARCH)/lib*.a $(SDL2_LIB_DIR)
	cp SDL2_ttf/$(ARCH)/lib*.a $(SDL2_LIB_DIR)

SDL2_image SDL2_mixer SDL2_gfx SDL2_ttf: SDL2
$(DIRS):
	@$(MAKE) -C $@

.PHONY: all clean $(DIRS)

clean:
	@for dir in $(DIRS); do \
		$(MAKE) -C $$dir -k clean; \
	done
