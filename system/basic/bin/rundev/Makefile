include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

RUNDEV_OBJS = rundev.o

RUNDEV = $(TARGET_DIR)/bin/rundev

$(RUNDEV): $(RUNDEV_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(RUNDEV_OBJS) -o $(RUNDEV) $(LDFLAGS) -lewoksys -lc -lgloss -lsoftfloat

clean:
	rm -f $(RUNDEV_OBJS)

