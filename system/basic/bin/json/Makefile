SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

JSON_OBJS = json.o

JSON = $(TARGET_DIR)/bin/json

$(JSON): $(JSON_OBJS) \
		$(BUILD_DIR)/lib/libtinyjson.a  \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(JSON_OBJS) -o $(JSON) $(LDFLAGS) -ltinyjson $(EWOK_LIBC)

clean:
	rm -f $(JSON_OBJS)

