SYS_DIR=../../..
include $(SYS_DIR)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I$(SYS_DIR)/basic/libs/libc/sys/include \
	-I$(SYS_DIR)/basic/libs/sd/include \
	-Iinclude -I $(TARGET_DIR)/include

LIB_BSP_OBJS = src/bsp_sd.o

BSP = $(TARGET_DIR)/lib/libbsp.a

$(BSP): $(LIB_BSP_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a \
		$(BUILD_DIR)/lib/libarch_miyoo.a
	$(AR) rT $(BSP) $(LIB_BSP_OBJS) $(BUILD_DIR)/lib/libarch_bcm283x.a $(BUILD_DIR)/lib/libarch_miyoo.a
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_BSP_OBJS)
	rm -f $(BSP)

