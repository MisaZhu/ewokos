SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

ACTLED_OBJS = actled.o

ACTLED = $(TARGET_DIR)/bin/actled

$(ACTLED): $(ACTLED_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(ACTLED_OBJS) -o $(ACTLED) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(ACTLED_OBJS)

