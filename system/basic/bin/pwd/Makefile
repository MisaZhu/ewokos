SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

PWD_OBJS = pwd.o

PWD = $(TARGET_DIR)/bin/pwd

$(PWD): $(PWD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(PWD_OBJS) -o $(PWD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(PWD_OBJS)

