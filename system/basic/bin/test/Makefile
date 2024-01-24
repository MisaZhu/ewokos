include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

TEST_OBJS = test.o

TEST = $(TARGET_DIR)/bin/test

$(TEST): $(TEST_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libgloss.a  \
		$(BUILD_DIR)/lib/libgloss.a
		$(LD) -Ttext=100 $(TEST_OBJS) -o $(TEST) $(LDFLAGS) -lewoksys -lc -lgloss 

clean:
	rm -f $(TEST_OBJS)

