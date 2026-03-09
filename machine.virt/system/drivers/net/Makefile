SYS_DIR=../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

NET_OBJS = net.o

NET = $(TARGET_DIR)/drivers/virt/net

$(NET): $(NET_OBJS) \
		$(EWOK_LIBC_A) \
		$(BUILD_DIR)/lib/libarch_virt.a
	$(LD) -Ttext=100 $(NET_OBJS) -o $(NET) $(LDFLAGS) $(BSP_LFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(NET_OBJS)
