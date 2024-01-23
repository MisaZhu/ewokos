include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

SOUNDD_OBJS = soundd.o

SOUNDD = $(TARGET_DIR)/drivers/raspix/soundd

$(SOUNDD): $(SOUNDD_OBJS) \
		$(BUILD_DIR)/lib/libewoksys.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(SOUNDD_OBJS) -o $(SOUNDD) $(LDFLAGS) -larch_bcm283x -lewoksys -lc -lgloss

clean:
	rm -f $(SOUNDD_OBJS)
