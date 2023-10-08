include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

JOYSTICKD_OBJS = joystickd.o

JOYSTICKD = $(TARGET_DIR)/drivers/miyoo/gpio_joystickd

$(JOYSTICKD): $(JOYSTICKD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a 
	$(LD) -Ttext=100 $(JOYSTICKD_OBJS) -o $(JOYSTICKD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(JOYSTICKD_OBJS)
