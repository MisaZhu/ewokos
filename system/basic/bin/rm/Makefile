SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

RM_OBJS = rm.o

RM = $(TARGET_DIR)/bin/rm

$(RM): $(RM_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(RM_OBJS) -o $(RM) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(RM_OBJS)

