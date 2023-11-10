include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

STL_TEST_OBJS = test.o

STL_TEST = $(TARGET_DIR)/bin/stl_test

$(STL_TEST): $(STL_TEST_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(STL_TEST_OBJS) -o $(STL_TEST) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(STL_TEST_OBJS) $(STL_TEST)