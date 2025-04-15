SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/arm/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libc/libewoksys/ewoksys/include -Iinclude -I $(TARGET_DIR)/include

LIB_ARCH_VPB_OBJS = src/framebuffer.o src/sd.o

ARCH_VPB = $(TARGET_DIR)/lib/libarch_vpb.a

$(ARCH_VPB): $(LIB_ARCH_VPB_OBJS)
	mkdir -p $(BUILD_DIR)
	$(AR) rcs $(ARCH_VPB) $(LIB_ARCH_VPB_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_ARCH_VPB_OBJS)
	rm -f $(ARCH_VPB)

