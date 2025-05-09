SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

LOGIN_OBJS = login.o

LOGIN = $(TARGET_DIR)/bin/login

$(LOGIN): $(LOGIN_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(LOGIN_OBJS) -o $(LOGIN) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(LOGIN_OBJS)

