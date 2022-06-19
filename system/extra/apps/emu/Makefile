SYS_DIR=../../..
include $(SYS_DIR)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs
LDFLAGS = -L$(BUILD_DIR)/lib -lnosys
CFLAGS += -I include -I $(BUILD_DIR)/include
CXXFLAGS += -I include -I $(BUILD_DIR)/include
EMU_OBJS = main.o src/MapperUxROM.o src/PPU.o src/Mapper.o src/PictureBus.o src/Cartridge.o src/Emulator.o src/MapperNROM.o src/MapperCNROM.o src/MainBus.o src/MapperSxROM.o src/VirtualScreen.o src/Controller.o src/CPU.o 
EMU_DIR = $(TARGET_DIR)/apps/emu
EMU = $(EMU_DIR)/emu

all: $(EMU_DIR) $(EMU)

$(EMU): $(EMU_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libcxx.a  
	$(LD) -Ttext=100 $(EMU_OBJS) -o $(EMU) $(LDFLAGS) -lfonts -lgraph -lx++ -lx -lewokc -lc -lcxx

$(EMU_DIR):
	mkdir -p $(EMU_DIR)
	cp -r res $(EMU_DIR)

clean:
	rm -f $(EMU_OBJS)
	rm -fr $(EMU_DIR)
