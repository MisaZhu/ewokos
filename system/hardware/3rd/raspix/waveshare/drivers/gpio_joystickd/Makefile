include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

JOYSTICKD_OBJS = joystickd.o

JOYSTICKD = $(TARGET_DIR)/drivers/raspix/gpio_joystickd

$(JOYSTICKD): $(JOYSTICKD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(JOYSTICKD_OBJS) -o $(JOYSTICKD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(JOYSTICKD_OBJS)
