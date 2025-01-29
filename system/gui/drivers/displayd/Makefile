include ../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -Iinclude -I $(BUILD_DIR)/include

DISPLAYD_OBJS = displayd.o

DISPLAYD = $(TARGET_DIR)/drivers/displayd

$(DISPLAYD): $(DISPLAYD_OBJS) \
		$(EWOK_LIBC_A)  
	$(LD) -Ttext=100 $(DISPLAYD_OBJS) -o $(DISPLAYD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(DISPLAYD_OBJS)
