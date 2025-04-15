SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_TINYJSON_OBJS = src/tinyjson.o

TINYJSON = $(TARGET_DIR)/lib/libtinyjson.a

$(TINYJSON): $(LIB_TINYJSON_OBJS)
	$(AR) rcs $(TINYJSON) $(LIB_TINYJSON_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_TINYJSON_OBJS)
	rm -f $(TINYJSON)

