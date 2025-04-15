SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_EWOKSTL_OBJS = src/string.o

EWOKSTL = $(TARGET_DIR)/lib/libewokstl.a

$(EWOKSTL): $(LIB_EWOKSTL_OBJS)
	$(AR) rcs $(EWOKSTL) $(LIB_EWOKSTL_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_EWOKSTL_OBJS)
	rm -f $(EWOKSTL)

