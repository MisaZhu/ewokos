SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SPLASH_OBJS = splash.o

SPLASH = $(TARGET_DIR)/bin/splash

$(SPLASH): $(SPLASH_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(SPLASH_OBJS) -o $(SPLASH) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(SPLASH_OBJS)

