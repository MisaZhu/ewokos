include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SYSINFOD_OBJS = sysinfod.o

SYSINFOD = $(TARGET_DIR)/drivers/proc/sysinfod

$(SYSINFOD): $(SYSINFOD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a 
	$(LD) -Ttext=100 $(SYSINFOD_OBJS) -o $(SYSINFOD) $(LDFLAGS) -lewoksys -lc -lgloss

clean:
	rm -f $(SYSINFOD_OBJS)
