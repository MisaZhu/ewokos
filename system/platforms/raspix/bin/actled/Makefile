include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

ACTLED_OBJS = actled.o

ACTLED = $(TARGET_DIR)/bin/actled

$(ACTLED): $(ACTLED_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a
	$(LD) -Ttext=100 $(ACTLED_OBJS) -o $(ACTLED) $(LDFLAGS) -lewokc -lc

clean:
	rm -f $(ACTLED_OBJS)

