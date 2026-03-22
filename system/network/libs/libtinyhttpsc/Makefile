SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

TASK = $(TARGET_DIR)/lib/libtinyhttpsc.a
TASK_OBJS = src/tinyhttpsc.o src/BearHttpsClientOne.o

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

all: $(TASK)

$(TASK): $(TASK_OBJS)
	$(AR) rcs $(TASK) $(TASK_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(TASK_OBJS)
