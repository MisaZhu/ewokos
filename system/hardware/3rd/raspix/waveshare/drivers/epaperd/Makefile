include ../../make.rule

BUILD_DIR = $(SYS_DIR)/build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

EPAPERD_OBJS = epaperd.o fbd.o

EPAPERD = $(TARGET_DIR)/drivers/raspix/epaperd

$(EPAPERD): $(EPAPERD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libarch_bcm283x.a 
	$(LD) -Ttext=100 $(EPAPERD_OBJS) -o $(EPAPERD) $(LDFLAGS) -larch_bcm283x -lfbd -lgraph -lewokc -lc

clean:
	rm -f $(EPAPERD_OBJS)
