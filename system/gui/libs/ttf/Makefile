SYS_DIR=../../../
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_TRUETY_OBJS = src/truety.o src/ttf.o

TRUETY = $(TARGET_DIR)/lib/libttf.a

$(TRUETY): $(LIB_TRUETY_OBJS)
	$(AR) rcs $(TRUETY) $(LIB_TRUETY_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_TRUETY_OBJS)
	rm -f $(TRUETY)

