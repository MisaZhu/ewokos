include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

TEST_OBJS = test.o

TEST = $(TARGET_DIR)/bin/test

$(TEST): $(TEST_OBJS) \
		$(EWOK_LIBC_A)
		$(LD) -Ttext=100 $(TEST_OBJS) -o $(TEST) $(LDFLAGS) $(EWOK_LIBC) 

clean:
	rm -f $(TEST_OBJS)

