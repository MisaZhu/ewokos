include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

DUPD_OBJS = dupd.o

DUPD = $(TARGET_DIR)/drivers/dupd

$(DUPD): $(DUPD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(DUPD_OBJS) -o $(DUPD) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(DUPD_OBJS)
