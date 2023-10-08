include ../../../../../hardware/$(ARCH)/make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

FBD_OBJS = fbd.o

FBD = $(TARGET_DIR)/drivers/miyoo/fbd

$(FBD): $(FBD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libupng.a \
		$(BUILD_DIR)/lib/libarch_miyoo.a
	$(LD) -Ttext=100 $(FBD_OBJS) -o $(FBD) $(LDFLAGS) -larch_miyoo -lfbd -lupng -lgraph -lewokc -lc

clean:
	rm -f $(FBD_OBJS)
