include ../../../../../make.rule

BUILD_DIR = ../../../../../build
TARGET_DIR = $(BUILD_DIR)/rootfs

LDFLAGS = -L$(BUILD_DIR)/lib -lnosys

CFLAGS += -I $(BUILD_DIR)/include

FBD_OBJS = fbd.o

FBD = $(TARGET_DIR)/drivers/rk3326/fbd

$(FBD): $(FBD_OBJS) \
		$(BUILD_DIR)/lib/libewokc.a \
		$(BUILD_DIR)/lib/libgraph.a \
		$(BUILD_DIR)/lib/libfbd.a \
		$(BUILD_DIR)/lib/libarch_rk3326.a
	$(LD) -Ttext=100 $(FBD_OBJS) -o $(FBD) $(LDFLAGS) -larch_rk3326 -lgraph -lfbd -lewokc -lc

clean:
	rm -f $(FBD_OBJS)
