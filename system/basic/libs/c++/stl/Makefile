SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_EWOKSTL_OBJS = src/string.o src/list.o src/deque.o src/set.o src/map.o src/unordered_set.o src/unordered_map.o src/new_delete.o src/iostream.o

EWOKSTL = $(TARGET_DIR)/lib/libewokstl.a

$(EWOKSTL): $(LIB_EWOKSTL_OBJS)
	$(AR) rcs $(EWOKSTL) $(LIB_EWOKSTL_OBJS)
	cp -r include/* $(TARGET_DIR)/include


clean:
	rm -f $(LIB_EWOKSTL_OBJS)
	rm -f $(EWOKSTL)
	rm -f src/iostream.o
