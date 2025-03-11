SYS_DIR=../../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -Llib 

CFLAGS += -I $(BUILD_DIR)/include

KBD_OBJS = joystickd.o

KBD = $(TARGET_DIR)/drivers/ev3/gpio_joystickd

$(KBD): $(KBD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_ev3.a
	$(LD) -Ttext=100 $(KBD_OBJS) -o $(KBD) $(LDFLAGS) -larch_ev3 $(EWOK_LIBC)

clean:
	rm -f $(KBD_OBJS)
