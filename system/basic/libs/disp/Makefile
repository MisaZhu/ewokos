include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CFLAGS += -I $(TARGET_DIR)/include -Iinclude

LIB_DISP_OBJS = src/disp.o

LIB_DISP = $(TARGET_DIR)/lib/libdisp.a

$(LIB_DISP): $(LIB_DISP_OBJS)
	$(AR) rT $(LIB_DISP) $(LIB_DISP_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_DISP_OBJS)
	rm -f $(LIB_DISP)

