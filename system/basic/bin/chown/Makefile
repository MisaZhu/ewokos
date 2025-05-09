SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CHOWN_OBJS = chown.o

CHOWN = $(TARGET_DIR)/bin/chown

$(CHOWN): $(CHOWN_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(CHOWN_OBJS) -o $(CHOWN) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(CHOWN_OBJS)

