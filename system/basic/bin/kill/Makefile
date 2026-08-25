SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

KILL_OBJS = $(ARCH)/kill.o

KILL = $(TARGET_DIR)/bin/kill

$(KILL): $(KILL_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(KILL_OBJS) -o $(KILL) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(KILL_OBJS)

