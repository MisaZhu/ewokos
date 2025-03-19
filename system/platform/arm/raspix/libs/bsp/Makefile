SYS_DIR=../../../../../
include $(SYS_DIR)/platform/arm/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libc/libewoksys/ewoksys/include \
	-I$(SYS_DIR)/basic/libs/sd/include \
	-I$(SYS_DIR)/basic/libs/gpio/include \
	-I$(SYS_DIR)/gui/libs/graph/include \
	-Iinclude -I $(TARGET_DIR)/include

ifeq ($(NEON),yes)
CFLAGS += -mfpu=neon-vfpv4 -DNEON_BOOST
CXXFLAGS += -mfpu=neon-vfpv4 -DNEON_BOOST
endif

LIB_BSP_OBJS = src/bsp_sd.o src/bsp_graph.o src/bsp_fb.o src/bsp_gpio.o

BSP = $(TARGET_DIR)/lib/libbsp.a

$(BSP): $(LIB_BSP_OBJS)
	$(AR) rcs $(BSP) $(LIB_BSP_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_BSP_OBJS)
	rm -f $(BSP)

