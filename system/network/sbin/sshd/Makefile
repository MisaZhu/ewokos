SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib -lwolfssl
CFLAGS += -I $(BUILD_DIR)/include -I $(BUILD_DIR)/include/wolfssl -DWOLFSSL_USER_SETTINGS -DSSHD_DEBUG

TASK_OBJS = $(ARCH)/sshd.o $(ARCH)/sftp-core.o
SFTP_SERVER_OBJS = $(ARCH)/sftp-server.o $(ARCH)/sftp-core.o

TASK = $(TARGET_DIR)/sbin/sshd
SFTP_SERVER = $(TARGET_DIR)/sbin/sftp-server

all: $(TASK) $(SFTP_SERVER)

$(TASK): $(TASK_OBJS) $(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libsocket.a \
		$(BUILD_DIR)/lib/libwolfssl.a
	$(LD) -Ttext=100 $(TASK_OBJS) -o $(TASK) $(LDFLAGS) -lsocket -lopenlibm $(EWOK_LIBC)

$(SFTP_SERVER): $(SFTP_SERVER_OBJS) $(BUILD_DIR)/lib/libewoksys.a
	$(LD) -Ttext=100 $(SFTP_SERVER_OBJS) -o $(SFTP_SERVER) -L $(BUILD_DIR)/lib $(EWOK_LIBC)

$(ARCH)/sftp-core.o: sftp-core.c sftp-core.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ sftp-core.c

$(ARCH)/sftp-server.o: sftp-server.c sftp-core.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ sftp-server.c

clean:
	rm -f $(TASK_OBJS)
	rm -f $(SFTP_SERVER_OBJS)
	rm -f $(TASK)
	rm -f $(SFTP_SERVER)
