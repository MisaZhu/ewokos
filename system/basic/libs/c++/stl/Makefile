SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

# Only the units with non-template definitions belong here.  set.o was dropped
# because <set> is now header-only: a class template's members cannot be defined in
# a single .cpp and still be instantiable for other types, and set.cpp's lone
# `template class set<int>;` meant std::set<anything else> failed to link.
# list.o and unordered_set.o have the same problem and go when those headers are
# converted.  string/new_delete/iostream are ordinary non-template code.
LIB_EWOKSTL_OBJS = $(ARCH)/src/string.o $(ARCH)/src/list.o $(ARCH)/src/unordered_set.o $(ARCH)/src/new_delete.o $(ARCH)/src/iostream.o

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

clean-headers:
	rm -f $(STL_HEADERS_STAMP)
	rm -rf $(TARGET_DIR)/include/*

rebuild: clean clean-headers all

.PHONY: all clean-headers rebuild