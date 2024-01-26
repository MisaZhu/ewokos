include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

XJOYSTICKD_OBJS = xjoystickd.o 

XJOYSTICKD = $(TARGET_DIR)/sbin/x/xjoystickd

$(XJOYSTICKD): $(XJOYSTICKD_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(XJOYSTICKD_OBJS) -o $(XJOYSTICKD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(XJOYSTICKD_OBJS)
