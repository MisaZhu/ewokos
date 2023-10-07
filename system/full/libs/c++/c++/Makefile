include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)

LDFLAGS = -L$(TARGET_DIR)/lib -lnosys

CXXFLAGS += -Iinclude -I $(TARGET_DIR)/include

LIB_CXX_OBJS= src/cxx.o

LIB_CXX = $(TARGET_DIR)/lib/libcxx.a

$(LIB_CXX): $(LIB_CXX_OBJS)
	$(AR) rT $(LIB_CXX) $(LIB_CXX_OBJS)


clean:
	rm -f $(LIB_CXX_OBJS)
	rm -f $(LIB_CXX)

