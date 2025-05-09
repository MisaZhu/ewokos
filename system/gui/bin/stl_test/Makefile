SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CXXFLAGS += -I $(BUILD_DIR)/include

STL_TEST_OBJS = test.o

STL_TEST = $(TARGET_DIR)/bin/stltest

$(STL_TEST): $(STL_TEST_OBJS) \
		$(EWOK_LIBC_A)  \
		$(BUILD_DIR)/lib/libcxx.a \
		$(BUILD_DIR)/lib/libewokstl.a 
	$(LD) -Ttext=100 $(STL_TEST_OBJS) -o $(STL_TEST) $(LDFLAGS) -lewokstl $(EWOK_LIBC) -lcxx -lstdc++

clean:
	rm -f $(STL_TEST_OBJS) $(STL_TEST)