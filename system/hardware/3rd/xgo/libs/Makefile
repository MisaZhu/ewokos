SYS_DIR = ../../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build

LDFLAGS += -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include -Iinclude

LIB_XGO_OBJS = src/xgo.o

XGO = $(BUILD_DIR)/lib/libxgo.a

$(XGO): $(LIB_XGO_OBJS)
	$(AR) rT $(XGO) $(LIB_XGO_OBJS)
	cp -r include/* $(BUILD_DIR)/include


clean:
	rm -f $(LIB_XGO_OBJS)
	rm -f $(XGO)

