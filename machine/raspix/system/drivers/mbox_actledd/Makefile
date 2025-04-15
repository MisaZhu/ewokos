SYS_DIR=../../../../../system
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

ACTLEDD_OBJS = actledd.o

ACTLEDD = $(TARGET_DIR)/drivers/raspix/mbox_actledd

$(ACTLEDD): $(ACTLEDD_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(ACTLEDD_OBJS) -o $(ACTLEDD) $(LDFLAGS) -larch_bcm283x $(EWOK_LIBC)

clean:
	rm -f $(ACTLEDD_OBJS)
