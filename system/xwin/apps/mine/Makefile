TASK = mine
TASK_OBJS = mine.o

SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_ROOT_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CXXFLAGS += -I $(BUILD_DIR)/include

APP_DIR = $(TARGET_ROOT_DIR)/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS)
	mkdir -p $(APP_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(APP_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) $(EWOK_LIBC) -lcxx
	if [ -d res ]; then cp -r res $(APP_DIR); fi

clean:
	rm -f $(TASK_OBJS)
	rm -fr $(APP_DIR)
