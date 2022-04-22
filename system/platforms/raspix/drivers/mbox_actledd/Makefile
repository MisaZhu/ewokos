include ../../../../make.rule

BUILD_DIR = ../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

ACTLEDD_OBJS = actledd.o

ACTLEDD = $(TARGET_DIR)/drivers/raspix/mbox_actledd

$(ACTLEDD): $(ACTLEDD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(ACTLEDD_OBJS) -o $(ACTLEDD) $(LDFLAGS) -larch_bcm283x -lewokc -lc

clean:
	rm -f $(ACTLEDD_OBJS)
