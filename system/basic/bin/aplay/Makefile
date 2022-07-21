include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

CAT_OBJS = aplay.o utils.o pcm.o

CAT = $(TARGET_DIR)/bin/aplay

$(CAT): $(CAT_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(CAT_OBJS) -o $(CAT) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(CAT_OBJS)