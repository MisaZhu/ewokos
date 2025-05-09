SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SVCINFO_OBJS = svcinfo.o

SVCINFO = $(TARGET_DIR)/bin/svcinfo

$(SVCINFO): $(SVCINFO_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(SVCINFO_OBJS) -o $(SVCINFO) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(SVCINFO_OBJS)

