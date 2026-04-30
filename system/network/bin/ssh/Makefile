SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lssl -lcrypto
CFLAGS += -I $(BUILD_DIR)/include

TASK_OBJS = main.o ssh.o

TASK = $(TARGET_DIR)/bin/ssh

$(TASK): $(TASK_OBJS) $(EWOK_LIBC_A) \
		$(BUILD_DIR)/lib/libssl.a \
		$(BUILD_DIR)/lib/libcrypto.a
	$(LD) -Ttext=100 $(TASK_OBJS) /Users/mingmingzhu/work/ewokos/system/network/libs/openssl/stubs.o -o $(TASK) $(LDFLAGS) -lsocket $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
	rm -f $(TASK)
