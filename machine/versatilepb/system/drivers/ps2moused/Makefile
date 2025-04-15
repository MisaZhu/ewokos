SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

PS2MOUSED_OBJS = ps2moused.o

PS2MOUSED = $(TARGET_DIR)/drivers/versatilepb/ps2moused

$(PS2MOUSED): $(PS2MOUSED_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(PS2MOUSED_OBJS) -o $(PS2MOUSED) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(PS2MOUSED_OBJS)
