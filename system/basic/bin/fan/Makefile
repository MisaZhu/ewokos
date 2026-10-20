SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

FAN_OBJS = $(ARCH)/fan.o

FAN = $(TARGET_DIR)/bin/fan

$(FAN): $(FAN_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(FAN_OBJS) -o $(FAN) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(FAN_OBJS)
