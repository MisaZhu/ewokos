SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

NFSFSD_OBJS = $(ARCH)/nfsfsd.o $(ARCH)/nfs.o $(ARCH)/rpc.o $(ARCH)/xdr.o

NFSFSD = $(TARGET_DIR)/drivers/nfsfsd

$(NFSFSD): $(NFSFSD_OBJS) \
		$(BUILD_DIR)/lib/libsocket.a \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(NFSFSD_OBJS) -o $(NFSFSD) $(LDFLAGS) -lsocket $(EWOK_LIBC)

clean:
	rm -f $(NFSFSD_OBJS)