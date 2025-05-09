SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SLEEP_OBJS = sleep.o

SLEEP = $(TARGET_DIR)/bin/sleep

$(SLEEP): $(SLEEP_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(SLEEP_OBJS) -o $(SLEEP) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(SLEEP_OBJS)

