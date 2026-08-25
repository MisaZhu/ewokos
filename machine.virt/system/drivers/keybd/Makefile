SYS_DIR=../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

KEYBD_OBJS = $(ARCH)/keybd.o

KEYBD = $(TARGET_DIR)/drivers/virt/keybd

$(KEYBD): $(KEYBD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libarch_virt.a \
		$(BUILD_DIR)/lib/libbsp.a
	$(LD) -Ttext=100 $(KEYBD_OBJS) -o $(KEYBD) $(LDFLAGS) $(BSP_LFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(KEYBD_OBJS)
