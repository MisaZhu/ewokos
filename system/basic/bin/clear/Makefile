include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CLEAR_OBJS = clear.o

CLEAR = $(TARGET_DIR)/bin/clear

$(CLEAR): $(CLEAR_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a
		$(LD) -Ttext=100 $(CLEAR_OBJS) -o $(CLEAR) $(LDFLAGS) -lewoksys -lc -lgloss 

clean:
	rm -f $(CLEAR_OBJS)

