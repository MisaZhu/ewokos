include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

LCDHATD_OBJS = lcdhatd.o fbd.o draw_logo.o

LCDHATD = $(TARGET_DIR)/drivers/raspix/lcdhatd

$(LCDHATD): $(LCDHATD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(LCDHATD_OBJS) -o $(LCDHATD) $(LDFLAGS) -larch_bcm283x -lgraph -lfonts -lupng -lewokc -lc

clean:
	rm -f $(LCDHATD_OBJS)
