SYS_DIR=../../..

include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = fbtest.o

TASK = $(TARGET_DIR)/bin/fbtest

$(TASK): $(TASK_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libfb.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfont.a  \
		$(BUILD_DIR)/lib/libdisplay.a 
	mkdir -p $(TARGET_DIR)/bin 
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lkeyb -lmouse $(EWOK_LIB_GRAPH) $(BSP_LFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
