SYS_DIR=../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libc/libewoksys/ewoksys/include -Iinclude -I $(TARGET_DIR)/include

LIB_ARCH_virt_OBJS = src/dma.o src/virtio.o src/virtfs.o src/framebuffer.o src/sd.o src/fw_cfg.o

ARCH_virt = $(TARGET_DIR)/lib/libarch_virt.a

$(ARCH_virt): $(LIB_ARCH_virt_OBJS)
	mkdir -p $(BUILD_DIR)
	$(AR) rcs $(ARCH_virt) $(LIB_ARCH_virt_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_ARCH_virt_OBJS)
	rm -f $(ARCH_virt)

