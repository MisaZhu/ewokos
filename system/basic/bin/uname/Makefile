include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

UNAME_OBJS = uname.o

UNAME = $(TARGET_DIR)/bin/uname

$(UNAME): $(UNAME_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(UNAME_OBJS) -o $(UNAME) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(UNAME_OBJS)

