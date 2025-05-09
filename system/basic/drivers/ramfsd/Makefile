SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

RAMFSD_OBJS = ramfsd.o

RAMFSD = $(TARGET_DIR)/drivers/ramfsd

$(RAMFSD): $(RAMFSD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(RAMFSD_OBJS) -o $(RAMFSD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(RAMFSD_OBJS)
