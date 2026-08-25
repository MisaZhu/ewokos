SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L $(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_DISPLAY_OBJS = $(ARCH)/src/display.o

DISPLAY = $(TARGET_DIR)/lib/libdisplay.a

$(DISPLAY): $(LIB_DISPLAY_OBJS)
	$(AR) rcs $(DISPLAY) $(LIB_DISPLAY_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_DISPLAY_OBJS)
	rm -f $(DISPLAY)
