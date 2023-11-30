include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

cp_OBJS = cp.o

cp = $(TARGET_DIR)/bin/cp

$(cp): $(cp_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(cp_OBJS) -o $(cp) $(LDFLAGS) -lewokc -lc_nano -lnosys

clean:
	rm -f $(cp_OBJS)

