include ../../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SYSINFOD_OBJS = sysinfod.o

SYSINFOD = $(TARGET_DIR)/drivers/proc/sysinfod

$(SYSINFOD): $(SYSINFOD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(SYSINFOD_OBJS) -o $(SYSINFOD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(SYSINFOD_OBJS)
