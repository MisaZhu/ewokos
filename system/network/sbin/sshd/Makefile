SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lwolfssl
CFLAGS += -I $(BUILD_DIR)/include -I $(BUILD_DIR)/include/wolfssl -DWOLFSSL_USER_SETTINGS

TASK_OBJS = sshd.o

TASK = $(TARGET_DIR)/sbin/sshd

$(TASK): $(TASK_OBJS) $(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libwolfssl.a
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lsocket -lm $(EWOK_LIBC)

clean:
	rm -f $(TASK_OBJS)
	rm -f $(TASK)
