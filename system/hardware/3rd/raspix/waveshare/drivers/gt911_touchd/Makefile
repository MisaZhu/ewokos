include ../../make.rule
BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/$(HW)

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

GT911_OBJS = gt911.o gt911_touchd.o

GT911 = $(TARGET_DIR)/drivers/raspix/gt911_touchd

$(GT911): $(GT911_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a
	$(LD) -Ttext=100 $(GT911_OBJS) -o $(GT911) $(LDFLAGS) -larch_bcm283x -lewokc -lc_nano -lnosys

clean:
	rm -f $(GT911_OBJS)
