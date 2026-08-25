SYS_DIR=../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -I $(SYS_DIR)/basic/libc/libewoksys/ewoksys/include \
	-I $(SYS_DIR)/basic/libs/sd/include \
	-I $(SYS_DIR)/basic/libs/gpio/include \
	-I $(SYS_DIR)/basic/libs/usb/include \
	-I $(SYS_DIR)/gui/libs/graph/include \
	-Iinclude -I $(TARGET_DIR)/include

LIB_BSP_OBJS = $(ARCH)/src/bsp_sd.o $(ARCH)/src/bsp_fb.o $(ARCH)/src/bsp_gpio.o $(ARCH)/src/bsp_usb.o

BSP = $(TARGET_DIR)/lib/libbsp.a

$(BSP): $(LIB_BSP_OBJS) \
	$(BUILD_DIR)/lib/libarch_virt.a
	$(AR) rcs $(BSP) $(LIB_BSP_OBJS)  $(BUILD_DIR)/lib/libarch_virt.a
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_BSP_OBJS)
	rm -f $(BSP)

