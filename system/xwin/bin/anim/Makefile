SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs/bin/x

LDFLAGS = -L $(BUILD_DIR)/lib
CXXFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = $(ARCH)/anim.o
TASK = anim

all: $(TASK)

$(TASK): $(TASK_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libopenlibm.a \
		$(BUILD_DIR)/lib/libcxx.a
	mkdir -p $(TARGET_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TARGET_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC) -lcxx

clean:
	rm -f $(TASK_OBJS)


