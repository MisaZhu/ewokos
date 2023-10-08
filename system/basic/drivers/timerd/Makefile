include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

TIMERD_OBJS = timerd.o

TIMERD = $(TARGET_DIR)/drivers/timerd

$(TIMERD): $(TIMERD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(TIMERD_OBJS) -o $(TIMERD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(TIMERD_OBJS)
