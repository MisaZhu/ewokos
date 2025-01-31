SYS_DIR=../../..

include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS += -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

JOYMOUSED_OBJS = joymoused.o

JOYMOUSED = $(TARGET_DIR)/drivers/joymoused

$(JOYMOUSED): $(JOYMOUSED_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(JOYMOUSED_OBJS) -o $(JOYMOUSED) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(JOYMOUSED_OBJS)
