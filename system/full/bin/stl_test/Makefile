include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

STL_TEST_OBJS = test.o

STL_TEST = $(TARGET_DIR)/bin/stl_test

$(STL_TEST): $(STL_TEST_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libcxx.a \
		$(BUILD_DIR)/lib/libewokstl.a 
	$(LD) -Ttext=100 $(STL_TEST_OBJS) -o $(STL_TEST) $(LDFLAGS) -lewokstl -lewokc -lc_nano -lnosys -lcxx -lstdc++

clean:
	rm -f $(STL_TEST_OBJS) $(STL_TEST)