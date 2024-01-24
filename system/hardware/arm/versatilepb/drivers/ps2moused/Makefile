include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

PS2MOUSED_OBJS = ps2moused.o

PS2MOUSED = $(TARGET_DIR)/drivers/versatilepb/ps2moused

$(PS2MOUSED): $(PS2MOUSED_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  
	$(LD) -Ttext=100 $(PS2MOUSED_OBJS) -o $(PS2MOUSED) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(PS2MOUSED_OBJS)
