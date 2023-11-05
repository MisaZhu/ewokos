SYS_DIR=../../..
include $(SYS_DIR)/hardware/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CXXFLAGS += -I $(BUILD_DIR)/include

GTEST_OBJS = gtest.o

GTEST_DIR = $(TARGET_DIR)/apps/gtest
GTEST = $(GTEST_DIR)/gtest

all: $(GTEST_DIR) $(GTEST)

$(GTEST): $(GTEST_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libx++.a \
		$(BUILD_DIR)/lib/libx.a \
		$(BUILD_DIR)/lib/libttf.a \
		$(BUILD_DIR)/lib/libfont.a \
		$(BUILD_DIR)/lib/libhash.a \
		$(BUILD_DIR)/lib/libcxx.a 
	$(LD) -Ttext=100 $(GTEST_OBJS) -o $(GTEST) $(LDFLAGS) -lgraph -lbsp -lttf -lfont -lhash -lx++ -lx -lupng -lewokc -lc -lcxx

$(GTEST_DIR):
	mkdir -p $(GTEST_DIR)
	cp -r res $(GTEST_DIR)

clean:
	rm -f $(GTEST_OBJS)
	rm -fr $(GTEST_DIR)

