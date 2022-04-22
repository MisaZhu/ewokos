include ../../../make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

PS_OBJS = ps.o

PS = $(TARGET_DIR)/bin/ps

$(PS): $(PS_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(PS_OBJS) -o $(PS) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(PS_OBJS)

