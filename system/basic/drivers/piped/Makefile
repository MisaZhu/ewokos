SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build_$(ARCH)/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L $(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

PIPED_OBJS = $(ARCH)/piped.o

PIPED = $(TARGET_DIR)/drivers/piped

$(ARCH)/piped.o: piped.c piped.h
	$(CC) $(CFLAGS) -c piped.c -o $@

$(PIPED): $(PIPED_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a 
	mkdir -p $(TARGET_DIR)/drivers/
	$(LD) -Ttext=100 $(PIPED_OBJS) -o $(PIPED) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(PIPED_OBJS)
