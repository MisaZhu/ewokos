include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

VKEYBD_OBJS = vkeybd.o

VKEYBD = $(TARGET_DIR)/drivers/vkeybd

$(VKEYBD): $(VKEYBD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(VKEYBD_OBJS) -o $(VKEYBD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(VKEYBD_OBJS)
