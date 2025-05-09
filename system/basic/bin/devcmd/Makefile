SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

DEVCMD_OBJS = devcmd.o

DEVCMD = $(TARGET_DIR)/bin/devcmd

$(DEVCMD): $(DEVCMD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(DEVCMD_OBJS) -o $(DEVCMD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(DEVCMD_OBJS)

