include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

CAT_OBJS = aplay.o utils.o pcm.o

CAT = $(TARGET_DIR)/bin/aplay

$(CAT): $(CAT_OBJS) \
		$(EWOK_LIBC_A) 
	$(LD) -Ttext=100 $(CAT_OBJS) -o $(CAT) $(LDFLAGS) $(EWOK_LIBC)

clean:
	rm -f $(CAT_OBJS) $(CAT)