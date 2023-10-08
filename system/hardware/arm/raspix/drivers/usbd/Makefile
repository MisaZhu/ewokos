include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -Llib -lnosys

CFLAGS += -I $(BUILD_DIR)/include -Iinclude

USBD_OBJS = usbd.o

USBD = $(TARGET_DIR)/drivers/raspix/usbd

$(USBD): $(USBD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(USBD_OBJS) -o $(USBD) $(LDFLAGS) -larch_bcm283x -lewokc -lcsud -lc

clean:
	rm -f $(USBD_OBJS)
