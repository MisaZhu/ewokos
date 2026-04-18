SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_EWOKSTL_OBJS = src/string.o src/list.o src/deque.o src/set.o src/unordered_set.o src/new_delete.o src/iostream.o

EWOKSTL = $(TARGET_DIR)/lib/libewokstl.a

STL_HEADERS_STAMP = $(TARGET_DIR)/include/.stl_headers_timestamp

all: $(EWOKSTL) $(STL_HEADERS_STAMP)

$(EWOKSTL): $(LIB_EWOKSTL_OBJS)
	$(AR) rcs $(EWOKSTL) $(LIB_EWOKSTL_OBJS)

$(STL_HEADERS_STAMP): include/*
	cp -r include/* $(TARGET_DIR)/include
	touch $(STL_HEADERS_STAMP)

$(LIB_EWOKSTL_OBJS): | $(STL_HEADERS_STAMP)

clean:
	rm -f $(LIB_EWOKSTL_OBJS)
	rm -f $(EWOKSTL)
	rm -f src/iostream.o

clean-headers:
	rm -f $(STL_HEADERS_STAMP)
	rm -rf $(TARGET_DIR)/include/*

rebuild: clean clean-headers all

.PHONY: all clean-headers rebuild