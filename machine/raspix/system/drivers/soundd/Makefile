SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SOUNDD_OBJS = soundd.o

SOUNDD = $(TARGET_DIR)/drivers/raspix/soundd

$(SOUNDD): $(SOUNDD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(SOUNDD_OBJS) -o $(SOUNDD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(SOUNDD_OBJS)
