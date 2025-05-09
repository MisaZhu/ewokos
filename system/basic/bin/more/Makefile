SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

more_OBJS = more.o

more = $(TARGET_DIR)/bin/more

$(more): $(more_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(more_OBJS) -o $(more) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(more_OBJS)

