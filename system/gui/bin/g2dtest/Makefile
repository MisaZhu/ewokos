SYS_DIR=../../..

include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(SYS_DIR)/gui/libs/g2dclient/include -I $(SYS_DIR)/gui/libs/graph/include -I $(BUILD_DIR)/include

TASK_OBJS = $(ARCH)/g2dtest.o

TASK = $(TARGET_DIR)/bin/g2dtest

$(TASK): $(TASK_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a  \
		$(BUILD_DIR)/lib/libg2dclient.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libdisplay.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libdisplayman.a
	mkdir -p $(TARGET_DIR)/bin
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) $(EWOK_LIB_GRAPH) -lg2dclient $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
