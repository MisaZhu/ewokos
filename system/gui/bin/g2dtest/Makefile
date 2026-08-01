SYS_DIR=../../..

include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = $(ARCH)/g2dtest.o

TASK = $(TARGET_DIR)/bin/g2dtest

$(TASK): $(TASK_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libg2d.a
	mkdir -p $(TARGET_DIR)/bin
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lg2d $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
