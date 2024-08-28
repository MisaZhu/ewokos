include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

JOYKEYBD_OBJS = joykeybd.o

JOYKEYBD = $(TARGET_DIR)/drivers/raspix/gpio_joykeybd

$(JOYKEYBD): $(JOYKEYBD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(JOYKEYBD_OBJS) -o $(JOYKEYBD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(JOYKEYBD_OBJS)
