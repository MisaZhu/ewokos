SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_displayLAY_OBJS = src/display.o

LIB_displayLAY = $(TARGET_DIR)/lib/libdisplay.a

$(LIB_displayLAY): $(LIB_displayLAY_OBJS)
	$(AR) rcs $(LIB_displayLAY) $(LIB_displayLAY_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_displayLAY_OBJS)
	rm -f $(LIB_displayLAY)

