include ../../../platform/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

MKDIR_OBJS = mkdir.o

MKDIR = $(TARGET_DIR)/bin/mkdir

$(MKDIR): $(MKDIR_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(MKDIR_OBJS) -o $(MKDIR) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(MKDIR_OBJS)

