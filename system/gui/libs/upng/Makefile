SYS_DIR=../../../
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_UPNG_OBJS = src/upng.o

UPNG = $(TARGET_DIR)/lib/libupng.a

$(UPNG): $(LIB_UPNG_OBJS)
	$(AR) rcs $(UPNG) $(LIB_UPNG_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_UPNG_OBJS)
	rm -f $(UPNG)

