include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

UNAME_OBJS = uname.o

UNAME = $(TARGET_DIR)/bin/uname

$(UNAME): $(UNAME_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(UNAME_OBJS) -o $(UNAME) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(UNAME_OBJS)

