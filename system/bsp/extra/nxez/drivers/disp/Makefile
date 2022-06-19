SYS_DIR=../../../../..
include $(SYS_DIR)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

DISPD_OBJS = dispd.o

DISPD = $(TARGET_DIR)/drivers/raspix/nxez_dispd

$(DISPD): $(DISPD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(DISPD_OBJS) -o $(DISPD) $(LDFLAGS) -larch_bcm283x -lewokc -lc

clean:
	rm -f $(DISPD_OBJS)
