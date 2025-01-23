ifeq ($(ARCH),)
export ARCH=arm
endif

include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_XGO_OBJS = src/xgo.o

XGO = $(TARGET_DIR)/lib/libxgo.a

$(XGO): $(LIB_XGO_OBJS)
	$(AR) rT $(XGO) $(LIB_XGO_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_XGO_OBJS)
	rm -f $(XGO)

