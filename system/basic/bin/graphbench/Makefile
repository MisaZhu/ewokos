SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

GRAPHBENCH_OBJS = graphbench.o

GRAPHBENCH = $(TARGET_DIR)/bin/graphbench

$(GRAPHBENCH): $(GRAPHBENCH_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgraph.a
	mkdir -p $(TARGET_DIR)/bin
	$(LD) -Ttext=100 $(GRAPHBENCH_OBJS) -o $(GRAPHBENCH) $(LDFLAGS) $(EWOK_LIB_GRAPH) $(EWOK_LIBC)

clean:
	rm -f $(GRAPHBENCH_OBJS)
