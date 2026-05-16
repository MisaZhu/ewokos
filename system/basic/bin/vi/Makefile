SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

VI_OBJS = vi.o

VI = $(TARGET_DIR)/bin/vi

$(VI): $(VI_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	$(LD) -Ttext=100 $(VI_OBJS) -o $(VI) $(LDFLAGS) $(EWOK_LIBC)

vi.o: vi.c $(BUILD_DIR)/include/setjmp.h

clean:
	rm -f $(VI_OBJS)
