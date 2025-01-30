SYS_DIR=../../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

JOYSTICKD_OBJS = joystickd.o

JOYSTICKD = $(TARGET_DIR)/drivers/rk3128/gpio_joystickd

$(JOYSTICKD): $(JOYSTICKD_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(JOYSTICKD_OBJS) -o $(JOYSTICKD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(JOYSTICKD_OBJS)
