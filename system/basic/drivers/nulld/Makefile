include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

NULLD_OBJS = nulld.o

NULLD = $(TARGET_DIR)/drivers/nulld

$(NULLD): $(NULLD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(NULLD_OBJS) -o $(NULLD) $(LDFLAGS) -lewokc -lc_nano -lnosys

clean:
	rm -f $(NULLD_OBJS)
