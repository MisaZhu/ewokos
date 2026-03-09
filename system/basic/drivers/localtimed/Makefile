SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

LOCALTIMED_OBJS = localtimed.o

LOCALTIMED = $(TARGET_DIR)/drivers/localtimed

$(LOCALTIMED): $(LOCALTIMED_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(LOCALTIMED_OBJS) -o $(LOCALTIMED) $(LDFLAGS) -lntpc -lsocket $(EWOK_LIBC)

clean:
	rm -f $(LOCALTIMED)
	rm -f $(LOCALTIMED_OBJS)
