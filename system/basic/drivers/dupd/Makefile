include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

DUPD_OBJS = dupd.o

DUPD = $(TARGET_DIR)/drivers/dupd

$(DUPD): $(DUPD_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(DUPD_OBJS) -o $(DUPD) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(DUPD_OBJS)
