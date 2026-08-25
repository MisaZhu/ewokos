SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_DISPLAYMAN_OBJS = $(ARCH)/src/displayman.o

LIB_DISPLAYMAN = $(TARGET_DIR)/lib/libdisplayman.a

$(LIB_DISPLAYMAN): $(LIB_DISPLAYMAN_OBJS)
	$(AR) rcs $(LIB_DISPLAYMAN) $(LIB_DISPLAYMAN_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_DISPLAYMAN_OBJS)
	rm -f $(LIB_DISPLAYMAN)
