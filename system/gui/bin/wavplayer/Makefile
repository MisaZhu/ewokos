SYS_DIR=../../..
include $(SYS_DIR)/platform/$(ARCH)/make.rule

BUILD_DIR = $(SYS_DIR)/build/$(HW)
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CAT_OBJS = wavplayer.o utils.o pcm.o

CAT = $(TARGET_DIR)/bin/wavplayer

$(CAT): $(CAT_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(CAT_OBJS) -o $(CAT) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(CAT_OBJS) $(CAT)