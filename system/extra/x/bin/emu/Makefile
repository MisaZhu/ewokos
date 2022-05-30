include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs
LDFLAGS = -L$(BUILD_DIR)/lib -lnosys
CFLAGS += -I include -I $(BUILD_DIR)/include
CXXFLAGS += -I include -I $(BUILD_DIR)/include
EMU_OBJS = main.o src/MapperUxROM.o src/PPU.o src/Mapper.o src/PictureBus.o src/Cartridge.o src/Emulator.o src/MapperNROM.o src/MapperCNROM.o src/MainBus.o src/MapperSxROM.o src/VirtualScreen.o src/Controller.o src/CPU.o 
EMU = $(TARGET_DIR)/bin/x/emu
ROM = $(TARGET_DIR)/data/roms/mario.nes

all: $(EMU) $(ROM)

$(EMU): $(EMU_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libcxx.a  
	$(LD) -Ttext=100 $(EMU_OBJS) -o $(EMU) $(LDFLAGS) -lfonts -lgraph -lx++ -lx -lewokc -lc -lcxx

$(ROM): mario.nes
	mkdir -p $(TARGET_DIR)/data/roms/
	cp mario.nes $(ROM)

clean:
	rm -f $(EMU_OBJS)
	rm -rf $(ROM)

