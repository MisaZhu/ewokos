include ../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib

CFLAGS += -I $(BUILD_DIR)/include

FONTD_OBJS = fontd.o

FONTD = $(TARGET_DIR)/drivers/fontd

$(FONTD): $(FONTD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libsconf.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libttf.a 
	$(LD) -Ttext=100 $(FONTD_OBJS) -o $(FONTD) $(LDFLAGS)  -lttf -lgraph -lbsp -lsconf -lewokc -lc_nano -lnosys

clean:
	rm -f $(FONTD_OBJS)
