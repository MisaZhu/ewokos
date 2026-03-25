SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_ROOT_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib
CXXFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = clock.o
TASK = clock

APP_DIR = $(TARGET_ROOT_DIR)/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) 
	mkdir -p $(APP_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(APP_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC) -lcxx
	cp -r res $(APP_DIR)

clean:
	rm -f $(TASK_OBJS)


