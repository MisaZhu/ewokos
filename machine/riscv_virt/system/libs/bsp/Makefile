SYS_DIR=../../../../../system/
include $(SYS_DIR)/platform/riscv/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libs/libc/ewoksys/include \
	-I$(SYS_DIR)/basic/libs/sd/include \
	-Iinclude -I $(TARGET_DIR)/include

LIB_BSP_OBJS = src/bsp_sd.o

BSP = $(TARGET_DIR)/lib/libbsp.a

$(BSP): $(LIB_BSP_OBJS) \
		$(BUILD_DIR)/lib/libarch_virt.a
	$(AR) rcs $(BSP) $(LIB_BSP_OBJS)  $(BUILD_DIR)/lib/libarch_virt.a
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_BSP_OBJS)
	rm -f $(BSP)

