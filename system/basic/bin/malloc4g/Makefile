SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MALLOC4G_OBJS = $(ARCH)/malloc4g.o

MALLOC4G = $(TARGET_DIR)/bin/malloc4g

$(MALLOC4G): $(MALLOC4G_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(MALLOC4G_OBJS) -o $(MALLOC4G) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(MALLOC4G_OBJS)
