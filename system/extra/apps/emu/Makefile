ifeq ($(ARCH),)
export ARCH=arm
endif

SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/extra
LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I include -I $(BUILD_DIR)/include
CXXFLAGS += -I include -I $(BUILD_DIR)/include
EMU_OBJS = main.o \
		src/InfoNES.o \
		src/InfoNES_Mapper.o \
		src/InfoNES_pAPU.o  \
		src/K6502.o

EMU_DIR = $(TARGET_DIR)/apps/emu
EMU = $(EMU_DIR)/emu

all: $(EMU_DIR) $(EMU)

$(EMU): $(EMU_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libcxx.a  
	$(LD) -Ttext=100 $(EMU_OBJS) -o $(EMU) $(LDFLAGS) -lx++ -lttf -lfont -lgraph -lbsp -lx -lsconf $(EWOK_LIBC) -lcxx

$(EMU_DIR):
	mkdir -p $(EMU_DIR)
	cp -r res $(EMU_DIR)

clean:
	rm -f $(EMU_OBJS)
	rm -fr $(EMU_DIR)
