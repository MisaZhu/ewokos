SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_DISPLAYD_OBJS = $(ARCH)/src/displayd.o

DISPLAYD = $(TARGET_DIR)/lib/libdisplayd.a

$(DISPLAYD): $(LIB_DISPLAYD_OBJS)
	$(AR) rcs $(DISPLAYD) $(LIB_DISPLAYD_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_DISPLAYD_OBJS)
	rm -f $(DISPLAYD)
