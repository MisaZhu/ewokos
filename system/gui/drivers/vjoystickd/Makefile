SYS_DIR=../../..

include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS += -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

VJOYSTICKD_OBJS = vjoystickd.o

VJOYSTICKD = $(TARGET_DIR)/drivers/vjoystickd

$(VJOYSTICKD): $(VJOYSTICKD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(VJOYSTICKD_OBJS) -o $(VJOYSTICKD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(VJOYSTICKD_OBJS)
