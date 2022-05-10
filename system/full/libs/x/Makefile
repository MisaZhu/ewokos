include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_X_OBJS = src/x.o \
	src/xwin.o \
	src/xwm.o

X = $(TARGET_DIR)/lib/libx.a

$(X): $(LIB_X_OBJS) \
		$(TARGET_DIR)/lib/libgraph.a \
		$(TARGET_DIR)/lib/libupng.a \
		$(TARGET_DIR)/lib/libfonts.a
	$(AR) rT $(X) $(LIB_X_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_X_OBJS)
	rm -f $(X)

