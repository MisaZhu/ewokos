include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

PS2KEYBD_OBJS = ps2keybd.o

PS2KEYBD = $(TARGET_DIR)/drivers/versatilepb/ps2keybd

$(PS2KEYBD): $(PS2KEYBD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(PS2KEYBD_OBJS) -o $(PS2KEYBD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(PS2KEYBD_OBJS)
