SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

IPCONFIG_OBJS = ipconfig.o

IPCONFIG = $(TARGET_DIR)/bin/ipconfig

$(IPCONFIG): $(IPCONFIG_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(IPCONFIG_OBJS) -o $(IPCONFIG) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(IPCONFIG_OBJS) $(IPCONFIG)	

