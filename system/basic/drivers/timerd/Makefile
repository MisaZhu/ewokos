SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TIMERD_OBJS = timerd.o

TIMERD = $(TARGET_DIR)/drivers/timerd

$(TIMERD): $(TIMERD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(TIMERD_OBJS) -o $(TIMERD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TIMERD_OBJS)
