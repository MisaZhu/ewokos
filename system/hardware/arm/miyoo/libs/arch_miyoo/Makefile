SYS_DIR=../../../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libs/libc/sys/include -Iinclude -I $(TARGET_DIR)/include

LIB_ARCH_MIYOO_OBJS = src/framebuffer.o \
	src/sdmmc.o \
	src/sd.o 

ARCH_BCM283X = $(TARGET_DIR)/lib/libarch_miyoo.a

$(ARCH_BCM283X): $(LIB_ARCH_MIYOO_OBJS)
	$(AR) rT $(ARCH_BCM283X) $(LIB_ARCH_MIYOO_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_ARCH_MIYOO_OBJS)
	rm -f $(ARCH_BCM283X)


