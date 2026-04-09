SYS_DIR=../../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_CXX_OBJS= src/cxx.o

LIB_CXX = $(TARGET_DIR)/lib/libcxx.a

$(LIB_CXX): $(LIB_CXX_OBJS)
	$(AR) rcs $(LIB_CXX) $(LIB_CXX_OBJS)


clean:
	rm -f $(LIB_CXX_OBJS)
	rm -f $(LIB_CXX)

