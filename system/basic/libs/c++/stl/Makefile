SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

# Only the units with non-template definitions belong here.  set.o and list.o
# were dropped because <set> and <list> are now header-only: a class template's
# members cannot be defined in a single .cpp and still be instantiable for other
# types, and each of those .cpp files ended in one explicit instantiation, so
# std::set<anything else> and std::list<anything else> failed to link.
# unordered_set.o has the same problem and goes when that header is converted.
# string/new_delete/iostream are ordinary non-template code, and cxx_runtime is
# the __cxa_* set that used to come from libcxx.a's cxx.o - see its comment for
# why having it here keeps that object from being extracted at all.
LIB_EWOKSTL_OBJS = $(ARCH)/src/string.o $(ARCH)/src/unordered_set.o $(ARCH)/src/new_delete.o $(ARCH)/src/cxx_runtime.o $(ARCH)/src/iostream.o

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