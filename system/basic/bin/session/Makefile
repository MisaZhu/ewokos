SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SESSION_OBJS = session.o

SESSION = $(TARGET_DIR)/bin/session

$(SESSION): $(SESSION_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(SESSION_OBJS) -o $(SESSION) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(SESSION_OBJS)

