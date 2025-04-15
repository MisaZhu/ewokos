SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TTYD_OBJS = ttyd.o

TTYD = $(TARGET_DIR)/drivers/versatilepb/ttyd

$(TTYD): $(TTYD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(TTYD_OBJS) -o $(TTYD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TTYD_OBJS) $(TTYD)
