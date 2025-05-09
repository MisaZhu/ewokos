SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_FBD_OBJS = src/fbd.o

FBD = $(TARGET_DIR)/lib/libfbd.a

$(FBD): $(LIB_FBD_OBJS)
	$(AR) rcs $(FBD) $(LIB_FBD_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_FBD_OBJS)
	rm -f $(FBD)

