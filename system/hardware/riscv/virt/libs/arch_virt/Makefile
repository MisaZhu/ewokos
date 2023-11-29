SYS_DIR=../../../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libs/libc/sys/include -Iinclude -I $(TARGET_DIR)/include

LIB_ARCH_VIRT_OBJS = src/sd.o 

ARCH_VIRT = $(TARGET_DIR)/lib/libarch_virt.a

$(ARCH_VIRT): $(LIB_ARCH_VIRT_OBJS)
	$(AR) rT $(ARCH_VIRT) $(LIB_ARCH_VIRT_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_ARCH_VIRT_OBJS)
	rm -f $(ARCH_VIRT)


