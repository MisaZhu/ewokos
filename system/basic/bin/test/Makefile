SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib
CFLAGS += -I $(BUILD_DIR)/include

TEST_OBJS = $(ARCH)/test.o
DMATEST_OBJS = $(ARCH)/dmatest.o
SHMCTEST_OBJS = $(ARCH)/shmctest.o

TEST = $(TARGET_DIR)/bin/test
DMATEST = $(TARGET_DIR)/bin/dmatest
SHMCTEST = $(TARGET_DIR)/bin/shmctest

.DEFAULT_GOAL := all
all: $(TEST) $(DMATEST) $(SHMCTEST)

$(TEST): $(TEST_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
		$(LD) -Ttext=100 $(TEST_OBJS) -o $(TEST) $(LDFLAGS) $(EWOK_LIBC) 

$(DMATEST): $(DMATEST_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
		$(LD) -Ttext=100 $(DMATEST_OBJS) -o $(DMATEST) $(LDFLAGS) $(EWOK_LIBC)

$(SHMCTEST): $(SHMCTEST_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a
		$(LD) -Ttext=100 $(SHMCTEST_OBJS) -o $(SHMCTEST) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(TEST_OBJS) $(DMATEST_OBJS) $(SHMCTEST_OBJS)