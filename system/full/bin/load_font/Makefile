include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

LOAD_FONT_OBJS = load_font.o

LOAD_FONT = $(TARGET_DIR)/bin/load_font


$(LOAD_FONT): $(LOAD_FONT_OBJS) $(EWOK_LIBC_A)
	$(LD) -Ttext=100 $(LOAD_FONT_OBJS) -o $(LOAD_FONT) $(LDFLAGS) -ltinyjson -lfont -lgraph -lbsp $(EWOK_LIBC)

clean:
	rm -f $(LOAD_FONT_OBJS) $(LOAD_FONT)