SYS_DIR=../../../../../
include $(SYS_DIR)/hardware/arm/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -I$(SYS_DIR)/basic/libs/libc/ewoksys/include \
	-I$(SYS_DIR)/basic/libs/sd/include \
	-I$(SYS_DIR)/full/libs/graph/include \
	-Iinclude -I $(TARGET_DIR)/include

ifeq ($(G_2D_BOOST),yes)
CFLAGS += -mfpu=neon-vfpv4 -DGRAPH_2D_BOOST
CXXFLAGS += -mfpu=neon-vfpv4 -DGRAPH_2D_BOOST
endif

LIB_BSP_OBJS = src/bsp_sd.o src/bsp_graph.o

BSP = $(TARGET_DIR)/lib/libbsp.a

$(BSP): $(LIB_BSP_OBJS) \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(AR) rT $(BSP) $(LIB_BSP_OBJS)  $(BUILD_DIR)/lib/libarch_bcm283x.a
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_BSP_OBJS)
	rm -f $(BSP)

