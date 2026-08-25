SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CORE_OBJS = $(ARCH)/core.o 
	
CORE = $(TARGET_DIR)/sbin/core

$(CORE): $(CORE_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(CORE_OBJS) -o $(CORE) $(LDFLAGS)  $(EWOK_LIBC) 

clean:
	rm -fr $(CORE) $(CORE_OBJS)