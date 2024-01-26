include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

JOYKEYBD_OBJS = joykeybd.o

JOYKEYBD = $(TARGET_DIR)/drivers/miyoo/gpio_joykeybd

$(JOYKEYBD): $(JOYKEYBD_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(JOYKEYBD_OBJS) -o $(JOYKEYBD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(JOYKEYBD_OBJS)
