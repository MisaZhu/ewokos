include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

GTEST_OBJS = gtest.o

GTEST = $(TARGET_DIR)/bin/x/gtest

$(GTEST): $(GTEST_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(GTEST_OBJS) -o $(GTEST) $(LDFLAGS) -lfonts -lgraphxx -lx++ -lupng -lewokc -lc -lcxx

clean:
	rm -f $(GTEST_OBJS)

