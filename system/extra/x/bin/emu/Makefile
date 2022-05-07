include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs
LDFLAGS = -L$(BUILD_DIR)/lib -lnosys
CFLAGS += -I nes -I $(BUILD_DIR)/include
CXXFLAGS += -I nes -I $(BUILD_DIR)/include
EMU_OBJS = main.o nes/cpu.o nes/ppu.o nes/cartridge.o nes/controller.o nes/memory_controller.o
EMU = $(TARGET_DIR)/bin/x/emu
ROM = $(TARGET_DIR)/data/roms/mario.nes

all: $(EMU) $(ROM)

$(EMU): $(EMU_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libcxx.a  
	$(LD) -Ttext=100 $(EMU_OBJS) -o $(EMU) $(LDFLAGS)  -lx++ -lgraph -lewokc -lc -lcxx

$(ROM): mario.nes
	mkdir -p $(TARGET_DIR)/data/roms/
	cp mario.nes $(ROM)

clean:
	rm -f $(EMU_OBJS)
	rm -rf $(ROM)

