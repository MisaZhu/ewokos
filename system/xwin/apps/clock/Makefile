SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = clock.o

TASK_DIR = $(TARGET_DIR)/apps/clock
TASK = $(TASK_DIR)/clock

all: $(TASK_DIR) $(TASK)

$(TASK): $(TASK_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libewokstl.a \
		$(BUILD_DIR)/lib/libwidget++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx

$(TASK_DIR):
	mkdir -p $(TASK_DIR)
	cp -r res $(TASK_DIR)

clean:
	rm -f $(TASK_OBJS)
	rm -fr $(TASK_DIR)

