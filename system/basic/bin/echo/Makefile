SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

ECHO_OBJS = echo.o

ECHO = $(TARGET_DIR)/bin/echo

$(ECHO): $(ECHO_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(ECHO_OBJS) -o $(ECHO) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(ECHO_OBJS)

