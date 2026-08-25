SYS_DIR=../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MOUSED_OBJS = $(ARCH)/moused.o

MOUSED = $(TARGET_DIR)/drivers/virt/moused

$(MOUSED): $(MOUSED_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libarch_virt.a \
		$(BUILD_DIR)/lib/libbsp.a
	$(LD) -Ttext=100 $(MOUSED_OBJS) -o $(MOUSED) $(LDFLAGS) $(BSP_LFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(MOUSED_OBJS)
