SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

RUNDEV_OBJS = ipcserv.o

RUNDEV = $(TARGET_DIR)/bin/ipcserv

$(RUNDEV): $(RUNDEV_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(RUNDEV_OBJS) -o $(RUNDEV) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(RUNDEV_OBJS)

