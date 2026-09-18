SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CAPTEST_OBJS = $(ARCH)/captest.o

CAPTEST = $(TARGET_DIR)/bin/captest

$(CAPTEST): $(CAPTEST_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(CAPTEST_OBJS) -o $(CAPTEST) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(CAPTEST_OBJS)
