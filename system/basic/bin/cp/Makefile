SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

cp_OBJS = cp.o

cp = $(TARGET_DIR)/bin/cp

$(cp): $(cp_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(cp_OBJS) -o $(cp) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(cp_OBJS)

