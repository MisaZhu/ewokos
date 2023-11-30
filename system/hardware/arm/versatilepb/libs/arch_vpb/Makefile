SYS_DIR=../../../../../
include $(SYS_DIR)/hardware/arm/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libs/libc/sys/include -Iinclude -I $(TARGET_DIR)/include

LIB_ARCH_VPB_OBJS = src/framebuffer.o src/sd.o

ARCH_VPB = $(TARGET_DIR)/lib/libarch_vpb.a

$(ARCH_VPB): $(LIB_ARCH_VPB_OBJS)
	$(AR) rT $(ARCH_VPB) $(LIB_ARCH_VPB_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_ARCH_VPB_OBJS)
	rm -f $(ARCH_VPB)

