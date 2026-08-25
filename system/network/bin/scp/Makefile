SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib -lwolfssl
CFLAGS += -I $(BUILD_DIR)/include -I $(BUILD_DIR)/include/wolfssl -DWOLFSSL_USER_SETTINGS

SCP_OBJS = $(ARCH)/scp.o $(ARCH)/ssh_support.o

SCP = $(TARGET_DIR)/bin/scp

$(SCP): $(SCP_OBJS) \
                $(BUILD_DIR)/lib/libewoksys.a \
                $(BUILD_DIR)/lib/libwolfssl.a \
                $(BUILD_DIR)/lib/libsocket.a \
                $(BUILD_DIR)/lib/libopenlibm.a
	$(LD) -Ttext=100 $(SCP_OBJS) -o $(SCP) $(LDFLAGS) -lsocket -lopenlibm $(EWOK_LIBC)

$(ARCH)/ssh_support.o: ../ssh/ssh.c ../ssh/ssh.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c ../ssh/ssh.c -o $@

clean:
	rm -f $(SCP_OBJS)
	rm -f $(SCP)
