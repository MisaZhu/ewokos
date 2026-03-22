SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_NTPC_OBJS = src/ntpc.o

NTPC = $(TARGET_DIR)/lib/libntpc.a

$(NTPC): $(LIB_NTPC_OBJS)
	$(AR) rcs $(NTPC) $(LIB_NTPC_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_NTPC_OBJS)
	rm -f $(NTPC)

