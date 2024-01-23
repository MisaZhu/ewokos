include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TIMERD_OBJS = timerd.o

TIMERD = $(TARGET_DIR)/drivers/timerd

$(TIMERD): $(TIMERD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(TIMERD_OBJS) -o $(TIMERD) $(LDFLAGS) -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(TIMERD_OBJS)
