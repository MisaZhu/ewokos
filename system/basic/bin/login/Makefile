include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

LOGIN_OBJS = login.o

LOGIN = $(TARGET_DIR)/bin/login

$(LOGIN): $(LOGIN_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(LOGIN_OBJS) -o $(LOGIN) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(LOGIN_OBJS)

