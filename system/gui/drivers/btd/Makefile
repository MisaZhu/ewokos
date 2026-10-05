SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include \
          -I $(SYS_DIR)/gui/libs/bt/include \
          -I $(SYS_DIR)/gui/libs/hid/include

BTD_OBJS = $(ARCH)/btd.o $(ARCH)/btd_known.o $(ARCH)/btd_classic.o \
           $(ARCH)/btd_l2cap.o $(ARCH)/btd_hid.o $(ARCH)/btd_le.o

BTD = $(TARGET_DIR)/drivers/$(HW)/btd

$(BTD): $(BTD_OBJS) \
		$(BUILD_DIR)/lib/libbsp.a \
		$(BUILD_DIR)/lib/libhid.a \
		$(BUILD_DIR)/lib/libewoksys.a
	mkdir -p $(TARGET_DIR)/drivers/$(HW)
	$(LD) -Ttext=100 $(BTD_OBJS) -o $(BTD) $(LDFLAGS) $(BSP_LFLAGS) -lhid $(EWOK_LIBC)

# All modules share the channel/session layout. Rebuild every object when it changes.
$(BTD_OBJS): btd_int.h

clean:
	rm -f $(BTD_OBJS) $(BTD)
