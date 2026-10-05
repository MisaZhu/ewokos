SYS_DIR=../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

# g2dclient / openlibm headers come from the source tree, not
# $(TARGET_DIR)/include: bsp is built in the basic_sys phase (via -C libs),
# which runs before $(SYS_ROOT_DIR)/basic installs openlibm's headers and
# before gui_sys installs libg2dclient's headers, so on a clean build the
# installed copies do not exist yet. g2d_arch.o (built here) includes
# <openlibm.h> under ARCH_BOOST, hence the openlibm source include.
CFLAGS += -I $(SYS_DIR)/basic/libc/libewoksys/ewoksys/include \
	-I $(SYS_DIR)/platform/$(ARCH)/include/$(ARCH) \
	-I $(SYS_DIR)/basic/libs/sd/include \
	-I $(SYS_DIR)/basic/libs/gpio/include \
	-I $(SYS_DIR)/gui/libs/usb/include \
	-I $(SYS_DIR)/gui/libs/hid/include \
	-I $(SYS_DIR)/gui/libs/bt/include \
	-I $(SYS_DIR)/basic/libs/openlibm/include \
	-I $(SYS_DIR)/basic/libs/openlibm/src \
	-I $(SYS_DIR)/gui/libs/graph/include \
	-I $(SYS_DIR)/gui/libs/g2dclient/include \
	-Iinclude -I $(TARGET_DIR)/include

LIB_BSP_OBJS = $(ARCH)/src/bsp_sd.o $(ARCH)/src/bsp_fb.o $(ARCH)/src/bsp_gpio.o $(ARCH)/src/bsp_usb.o $(ARCH)/src/bsp_bt.o \
	$(ARCH)/src/bsp_g2d.o $(SYS_DIR)/platform/$(ARCH)/src/g2d_arch.o

BSP = $(TARGET_DIR)/lib/libbsp.a

$(BSP): $(LIB_BSP_OBJS) \
	$(BUILD_DIR)/lib/libarch_virt.a
	$(AR) rcs $(BSP) $(LIB_BSP_OBJS)  $(BUILD_DIR)/lib/libarch_virt.a
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_BSP_OBJS)
	rm -f $(BSP)

