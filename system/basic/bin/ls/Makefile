SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

LS_OBJS = ls.o

LS = $(TARGET_DIR)/bin/ls

$(LS): $(LS_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(LS_OBJS) -o $(LS) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(LS_OBJS)

