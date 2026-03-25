SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

TARGET_DIR = $(SYS_DIR)/build/$(HW)
BUILD_DIR = $(TARGET_DIR)/rootfs

LDFLAGS = -L$(TARGET_DIR)/lib
CXXFLAGS += -I $(TARGET_DIR)/include

TASK_OBJS = clock.o
TASK = clock

APP_DIR = $(BUILD_DIR)/apps/$(TASK)

all: $(TASK)

$(TASK): $(TASK_OBJS) 
	mkdir -p $(APP_DIR)
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(APP_DIR)/$(TASK) $(LDFLAGS) $(EWOK_LIB_X) $(EWOK_LIB_GRAPH) -lopenlibm $(EWOK_LIBC) -lcxx
	cp -r res $(APP_DIR)

clean:
	rm -f $(TASK_OBJS)


