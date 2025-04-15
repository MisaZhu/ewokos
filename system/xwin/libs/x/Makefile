SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I$(TARGET_DIR)/include

LIB_X_OBJS = src/x.o \
	src/xwin.o \
	src/xwm.o

X = $(TARGET_DIR)/lib/libx.a

$(X): $(LIB_X_OBJS)
	$(AR) rcs $(X) $(LIB_X_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_X_OBJS)
	rm -f $(X)

