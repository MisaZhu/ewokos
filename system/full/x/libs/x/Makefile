include ../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I$(TARGET_DIR)/include

LIB_X_OBJS = src/x.o \
	src/xwin.o \
	src/xwm.o

X = $(TARGET_DIR)/lib/libx.a
LIB_TINYJSON = $(TARGET_DIR)/lib/libtinyjson.a

$(X): $(LIB_X_OBJS)
	$(AR) rT $(X) $(LIB_TINYJSON) $(LIB_X_OBJS)
	cp -r include/* $(TARGET_DIR)/include

clean:
	rm -f $(LIB_X_OBJS)
	rm -f $(X)

