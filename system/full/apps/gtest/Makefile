SYS_DIR=../../..
include $(SYS_DIR)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

GTEST_OBJS = gtest.o

GTEST_DIR = $(TARGET_DIR)/apps/gtest
GTEST = $(GTEST_DIR)/gtest

$(GTEST): $(GTEST_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(GTEST_OBJS) -o $(GTEST) $(LDFLAGS) -lfonts -lgraph -lx++ -lx -lupng -lewokc -lc -lcxx
	cp -r data $(GTEST_DIR)

clean:
	rm -f $(GTEST_OBJS)

