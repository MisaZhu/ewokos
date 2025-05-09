SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_TASK_OBJS = src/mouse.o

TASK = $(TARGET_DIR)/lib/libmouse.a

$(TASK): $(LIB_TASK_OBJS)
	$(AR) rcs $(TASK) $(LIB_TASK_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_TASK_OBJS)
	rm -f $(TASK)

